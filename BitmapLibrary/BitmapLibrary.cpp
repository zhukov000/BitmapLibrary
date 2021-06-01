// BitmapLibrary.cpp - Defines the exported functions for the DLL

#include "pch.h"
#include "BitmapLibrary.h"

BitmapFile::BitmapFile(std::string FileName) {
	std::ifstream fin{ FileName , std::ios::binary };
	
	if (fin) {
		// read header
		char bmpfh[BMPFileHeaderSize];
		fin.read(reinterpret_cast<char*>(&bmpfh), BMPFileHeaderSize);
		bmpFileHeader.set(bmpfh);

		if (bmpFileHeader.bfType != BMPT) {
			throw std::runtime_error("Unrecognized file format.");
		}

		char bmpih[BMPInfoHeaderSize];
		fin.read(reinterpret_cast<char*>(&bmpih), BMPInfoHeaderSize);
		bmpInfoHeader.set(bmpih);

		// for transparent image
		if (bmpInfoHeader.biBitCount == CR_HighColorsTransparent) {
			
			// check bit mask color information
			if ( bmpInfoHeader.biSize >= BMPInfoHeaderSize + sizeof(BitmapColorHeader) ) { 
				 
				bmpColorHeader.check();
			}
			else {
				throw std::runtime_error("Unrecognized file format.");
			}
		}

		// Jump to the pixel data location
		fin.seekg(bmpFileHeader.bfOffBits, fin.beg);

		if (bmpInfoHeader.biBitCount == CR_HighColorsTransparent) {
			bmpInfoHeader.biSize = BMPInfoHeaderSize + sizeof(BitmapColorHeader);
			bmpFileHeader.bfOffBits = BMPFileHeaderSize + BMPInfoHeaderSize + sizeof(BitmapColorHeader);
		}
		else {
			bmpInfoHeader.biSize = BMPInfoHeaderSize;
			bmpFileHeader.bfOffBits = BMPFileHeaderSize + BMPInfoHeaderSize;
		}
		bmpFileHeader.bfSize = bmpFileHeader.bfOffBits;

		if (bmpInfoHeader.biHeight < 0) {
			throw std::runtime_error("The program can treat only BMP images with the origin in the bottom left corner!");
		}

		bmpData.resize( bmpInfoHeader.biWidth * bmpInfoHeader.biHeight * bmpInfoHeader.biBitCount / 8 );

		// Here we check if we need to take into account row padding
		if (bmpInfoHeader.biWidth % 4 == 0) {
			fin.read(reinterpret_cast<char*>(bmpData.data()), bmpData.size());
			bmpFileHeader.bfSize += bmpData.size();
		}
		else {
			row_stride = bmpInfoHeader.biWidth * bmpInfoHeader.biBitCount / 8;
			uint32_t new_stride = make_stride_aligned(4);
			std::vector<uint8_t> padding_row(new_stride - row_stride);

			for (uint32_t y = 0; y < bmpInfoHeader.biHeight; ++y) {
				fin.read(reinterpret_cast<char*>(bmpData.data() + row_stride * y), row_stride);
				fin.read(reinterpret_cast<char*>(padding_row.data()), padding_row.size());
			}
			bmpFileHeader.bfSize += bmpData.size() + bmpInfoHeader.biHeight * padding_row.size();
		}
	}
	else {
		throw std::runtime_error("Unable to open the input file.");
	}
}

BitmapFile::BitmapFile(int32_t width, int32_t height, bool has_alpha) {
	if (width <= 0 || height <= 0) {
		throw std::runtime_error("The image width and height must be positive numbers.");
	}

	bmpInfoHeader.biWidth = width;
	bmpInfoHeader.biHeight = height;
	if (has_alpha) {
		bmpInfoHeader.biSize = BMPInfoHeaderSize + sizeof(BitmapColorHeader);
		bmpFileHeader.bfOffBits = BMPFileHeaderSize + BMPInfoHeaderSize + sizeof(BitmapColorHeader);

		bmpInfoHeader.biBitCount = CR_HighColorsTransparent;
		bmpInfoHeader.biCompression = 3;
		row_stride = width * 4;
		bmpData.resize(row_stride * height);
		bmpFileHeader.bfSize = bmpFileHeader.bfOffBits + bmpData.size();
	}
	else {
		bmpInfoHeader.biSize = BMPInfoHeaderSize;
		bmpFileHeader.bfOffBits = BMPFileHeaderSize + BMPInfoHeaderSize;

		bmpInfoHeader.biBitCount = CR_HighColors;
		bmpInfoHeader.biCompression = 0;
		row_stride = width * 3;
		bmpData.resize(row_stride * height);

		uint32_t new_stride = make_stride_aligned(4);
		bmpFileHeader.bfSize = bmpFileHeader.bfOffBits + bmpData.size() + bmpInfoHeader.biHeight * (new_stride - row_stride);
	}
}

/// <summary>
/// Fill region
/// </summary>
void BitmapFile::fillRegion(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint8_t B, uint8_t G, uint8_t R, uint8_t A) {
	if (x0 + w > static_cast<uint32_t>(bmpInfoHeader.biWidth) || y0 + h > static_cast<uint32_t>(bmpInfoHeader.biHeight)) {
		throw std::runtime_error("The region does not fit in the image!");
	}

	uint32_t channels = bmpInfoHeader.biBitCount / 8;
	for(uint32_t y = y0; y < y0 + h; ++y) {
		for (uint32_t x = x0; x < x0 + w; ++x) {
			bmpData[channels * (y * bmpInfoHeader.biWidth + x) + 0] = B;
			bmpData[channels * (y * bmpInfoHeader.biWidth + x) + 1] = G;
			bmpData[channels * (y * bmpInfoHeader.biWidth + x) + 2] = R;
			if (channels == 4) {
				bmpData[channels * (y * bmpInfoHeader.biWidth + x) + 3] = A;
			}
		}
	}
}

/// <summary>
/// Check if the pixel data is stored as BGRA and if the color space type is sRGB
/// </summary>
void BitmapColorHeader::check() {
	BitmapColorHeader expected_color_header;
	if (expected_color_header.red_mask != red_mask ||
		expected_color_header.blue_mask != blue_mask ||
		expected_color_header.green_mask != green_mask ||
		expected_color_header.alpha_mask != alpha_mask) {
		throw std::runtime_error("Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
	}
	if (expected_color_header.color_space_type != color_space_type) {
		throw std::runtime_error("Unexpected color space type! The program expects sRGB values");
	}
}

void BitmapFileHeader::set(char* rawData) {
	bfType = *(reinterpret_cast<uint16_t*>(rawData));
	bfSize = *(reinterpret_cast<uint32_t*>(rawData + 2));
	bfReserved1 = *(reinterpret_cast<uint16_t*>(rawData + 6));
	bfReserved2 = *(reinterpret_cast<uint16_t*>(rawData + 8));
	bfOffBits = *(reinterpret_cast<uint32_t*>(rawData + 10));
}

char* BitmapFileHeader::get() {
	char* rawData = new char[BMPFileHeaderSize];
	uint16_t* p16 = reinterpret_cast<uint16_t*>(rawData);
	uint32_t* p32 = reinterpret_cast<uint32_t*>(rawData + 2);
	p16[0] = bfType;
	p32[0] = bfSize;
	p16[3] = bfReserved1;
	p16[4] = bfReserved2;
	p32[2] = bfOffBits;
	return rawData;
}

void BitmapInfoHeader::set(char* rawData) {
	biSize = *(reinterpret_cast<uint32_t*>(rawData));
	biWidth = *(reinterpret_cast<uint32_t*>(rawData + 4));
	biHeight = *(reinterpret_cast<uint32_t*>(rawData + 8));
	biPlanes = *(reinterpret_cast<uint16_t*>(rawData + 12));
	biBitCount = *(reinterpret_cast<ColorResolution*>(rawData + 14));
	biCompression = *(reinterpret_cast<uint32_t*>(rawData + 16));
	biSizeImage = *(reinterpret_cast<uint32_t*>(rawData + 20));
	biXPelsPerMeter = *(reinterpret_cast<uint32_t*>(rawData + 24));
	biYPelsPerMeter = *(reinterpret_cast<uint32_t*>(rawData + 28));
	biClrUsed = *(reinterpret_cast<uint32_t*>(rawData + 32));
	biClrImportant = *(reinterpret_cast<uint32_t*>(rawData + 36));
}

char* BitmapInfoHeader::get() {
	char* rawData = new char[BMPInfoHeaderSize];
	
	uint16_t* p16 = reinterpret_cast<uint16_t*>(rawData + 12);
	p16[0] = biPlanes;
	p16[1] = biBitCount;

	uint32_t* p32 = reinterpret_cast<uint32_t*>(rawData);
	p32[0] = biSize;
	p32[1] = biWidth;
	p32[2] = biHeight;
	p32[4] = biCompression;
	p32[5] = biSizeImage;
	p32[6] = biXPelsPerMeter;
	p32[7] = biYPelsPerMeter;
	p32[8] = biClrUsed;
	p32[9] = biClrImportant;
	
	return rawData;
}

BitmapFile LoadBMP(std::string FileName) {
	return BitmapFile(FileName);
}

BitmapFile CreateBMP(int32_t width, int32_t height, bool hasAlpha = true) {
	return BitmapFile(width, height, hasAlpha);
}

void SaveBMP(std::string FileName, BitmapFile File) {
	// TODO
}