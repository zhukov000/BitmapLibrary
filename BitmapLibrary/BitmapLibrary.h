// BitmapLibrary.h - contains declaration structure and methods for BMP processing
#pragma once

#include<string>
#include<vector>
#include<fstream>

#ifdef BITMAPLIBRARY_EXPORTS
#define BITMAPLIBRARY_API __declspec(dllexport)
#else
#define BITMAPLIBRARY_API __declspec(dllimport)
#endif

/**
*
* BMP description: 
* https://web.archive.org/web/20080912171714/http://www.fortunecity.com/skyscraper/windows/364/bmpffrmt.html
* 
* Use:
* https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
* 
*/

const uint16_t BMPT = 0x4d42;
const uint16_t BMPFileHeaderSize = 14;
const uint16_t BMPInfoHeaderSize = 40;

struct BitmapFileHeader {
	// must always be set to 'BM' to declare that this is a .bmp-file
	uint16_t bfType{ 0 };
	// specifies the size of the file in bytes
	uint32_t bfSize{ 0 };
	// must always be set to zero
	uint16_t bfReserved1 { 0 };
	// must always be set to zero
	uint16_t bfReserved2 { 0 };
	// specifies the offset from the beginning of the file to the bitmap data
	uint32_t bfOffBits{ 0 };

	void set(char*);
	char* get();
};

enum ColorResolution: uint16_t {
	CR_BlackWhite = 1,
	CR_16Colors = 4,
	CR_256Colors = 8,
	CR_HighColors = 24,
	CR_HighColorsTransparent = 32,
};

struct BitmapInfoHeader {
	// specifies the size of the BITMAPINFOHEADER structure, in bytes
	uint32_t biSize{ 0 };
	// specifies the width of the image, in pixels
	uint32_t biWidth{ 0 };
	// specifies the height of the image, in pixels
	uint32_t biHeight{ 0 };

	// specifies the number of planes of the target device, must be set to zero
	uint16_t biPlanes{ 1 };
	// specifies the number of bits per pixel
	ColorResolution biBitCount = CR_256Colors;
	// Specifies the type of compression, usually set to zero (no compression)
	uint32_t biCompression { 0 };
	// specifies the size of the image data, in bytes. If there is no compression, it is valid to set this member to zero
	uint32_t biSizeImage { 0 };
	// specifies the the horizontal pixels per meter on the designated targer device, usually set to zero
	uint32_t biXPelsPerMeter { 0 };
	// specifies the the vertical pixels per meter on the designated targer device, usually set to zero
	uint32_t biYPelsPerMeter { 0 };
	// specifies the number of colors used in the bitmap, if set to zero the number of colors is calculated using the biBitCount member
	uint32_t biClrUsed { 0 };
	// specifies the number of color that are 'important' for the bitmap, if set to zero, all colors are important
	uint32_t biClrImportant { 0 };

	void set(char*);
	char* get();
};

#pragma pack(push, 1)
struct RGBQuad {
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
	// must always be set to zero
	uint8_t rgbReserved { 0 };
};
#pragma pack(pop)

struct BitmapColorHeader {
	uint32_t red_mask{ 0x00ff0000 };
	uint32_t green_mask{ 0x0000ff00 };
	uint32_t blue_mask{ 0x000000ff };
	uint32_t alpha_mask{ 0xff000000 };
	// Default "sRGB" (0x73524742)
	uint32_t color_space_type{ 0x73524742 };
	// Unused data for sRGB color space
	uint32_t unused[16]{ 0 };

	void check();
};

struct BitmapFile {

public:
	BitmapFileHeader bmpFileHeader;
	BitmapInfoHeader bmpInfoHeader;
	BitmapColorHeader bmpColorHeader;
	std::vector<uint8_t> bmpData;

	// BitmapFile(const BitmapFile& Copy);
	BitmapFile(std::string FileName);
	BitmapFile(int32_t width, int32_t height, bool has_alpha = true);

	void fillRegion(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint8_t B, uint8_t G, uint8_t R, uint8_t A);

private:
	uint32_t row_stride{ 0 };

	/// <summary>
	/// 
	/// </summary>
	/// <param name="of"></param>
	void write_headers(std::ofstream& of) {
		of.write(reinterpret_cast<const char*>(&bmpFileHeader), sizeof(bmpFileHeader));
		of.write(reinterpret_cast<const char*>(&bmpInfoHeader), sizeof(bmpInfoHeader));
		if (bmpInfoHeader.biBitCount == CR_HighColorsTransparent) {
			of.write(reinterpret_cast<const char*>(&bmpColorHeader), sizeof(bmpColorHeader));
		}
	}
	
	/// <summary>
	/// 
	/// </summary>
	/// <param name="of"></param>
	void write_headers_and_data(std::ofstream& of) {
		write_headers(of);
		of.write(reinterpret_cast<const char*>(bmpData.data()), bmpData.size());
	}

	/// <summary>
	/// Add 1 to the row_stride until it is divisible with align_stride
	/// </summary>
	/// <param name="align_stride"></param>
	/// <returns></returns>
	uint32_t make_stride_aligned(uint32_t align_stride) {
		uint32_t new_stride = row_stride;
		while (new_stride % align_stride != 0) {
			new_stride++;
		}
		return new_stride;
	}
};

extern "C++" BITMAPLIBRARY_API BitmapFile LoadBMP(std::string FileName);

extern "C++" BITMAPLIBRARY_API void SaveBMP(std::string FileName, BitmapFile File);
