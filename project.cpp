
#include <iostream>
#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <vector>
#include <string>

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")

using namespace std;
using namespace Gdiplus;

// Pixel structure
struct Pixel {
    BYTE R, G, B;
};

// Image matrix
using PixelMatrix = vector<vector<Pixel>>;


// Get JPEG encoder
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;
    UINT size = 0;

    GetImageEncodersSize(&num, &size);

    if (size == 0)
        return -1;

    vector<BYTE> buffer(size);

    ImageCodecInfo* codecs =
        reinterpret_cast<ImageCodecInfo*>(buffer.data());

    GetImageEncoders(num, size, codecs);

    for (UINT i = 0; i < num; i++) {
        if (wcscmp(codecs[i].MimeType, format) == 0) {
            *pClsid = codecs[i].Clsid;
            return i;
        }
    }

    return -1;
}


// Choose JPG image
wstring ChooseImage()
{
    OPENFILENAMEW ofn = {};
    WCHAR fileName[MAX_PATH] = L"";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        L"JPEG Images (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
        return fileName;

    return L"";
}


// Read image into matrix
PixelMatrix ReadImageMatrix(
    const wstring& path,
    int& width,
    int& height)
{
    Bitmap image(path.c_str());

    width = image.GetWidth();
    height = image.GetHeight();

    PixelMatrix pixels(
        height,
        vector<Pixel>(width)
    );

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            Color color;

            image.GetPixel(j, i, &color);

            pixels[i][j].R = color.GetR();
            pixels[i][j].G = color.GetG();
            pixels[i][j].B = color.GetB();
        }
    }

    return pixels;
}


// Save matrix as JPG
bool SaveImage(
    const PixelMatrix& pixels,
    int width,
    int height,
    const wstring& path)
{
    Bitmap bitmap(width, height, PixelFormat24bppRGB);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            Color color(
                pixels[i][j].R,
                pixels[i][j].G,
                pixels[i][j].B
            );

            bitmap.SetPixel(j, i, color);
        }
    }

    CLSID encoder;

    if (GetEncoderClsid(L"image/jpeg", &encoder) < 0)
        return false;

    DeleteFileW(path.c_str());

    return bitmap.Save(path.c_str(), &encoder, NULL) == Ok;
}


// Grayscale
PixelMatrix Grayscale(
    const PixelMatrix& original,
    int height,
    int width)
{
    PixelMatrix result = original;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            int gray = (int)(
                0.299 * original[i][j].R +
                0.587 * original[i][j].G +
                0.114 * original[i][j].B
            );

            result[i][j].R = (BYTE)gray;
            result[i][j].G = (BYTE)gray;
            result[i][j].B = (BYTE)gray;
        }
    }

    return result;
}


// Brightness adjustment
PixelMatrix Brightness(
    const PixelMatrix& original,
    int height,
    int width,
    int amount)
{
    PixelMatrix result = original;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            int red = (int)original[i][j].R + amount;
            int green = (int)original[i][j].G + amount;
            int blue = (int)original[i][j].B + amount;

            // Keep values between 0 and 255
            if (red > 255) red = 255;
            if (red < 0) red = 0;

            if (green > 255) green = 255;
            if (green < 0) green = 0;

            if (blue > 255) blue = 255;
            if (blue < 0) blue = 0;

            result[i][j].R = (BYTE)red;
            result[i][j].G = (BYTE)green;
            result[i][j].B = (BYTE)blue;
        }
    }

    return result;
}


// Horizontal Flip
PixelMatrix HorizontalFlip(
    const PixelMatrix& original,
    int height,
    int width)
{
    PixelMatrix result = original;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            result[i][j] =
                original[i][width - 1 - j];
        }
    }

    return result;
}


// Black and White
PixelMatrix BlackWhite(
    const PixelMatrix& original,
    int height,
    int width,
    int threshold = 127)
{
    PixelMatrix result = original;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            int gray = (int)(
                0.299 * original[i][j].R +
                0.587 * original[i][j].G +
                0.114 * original[i][j].B
            );

            BYTE value;

            if (gray >= threshold)
                value = 255;
            else
                value = 0;

            result[i][j].R = value;
            result[i][j].G = value;
            result[i][j].B = value;
        }
    }

    return result;
}


// Display image using Windows viewer
void DisplayImage(const wstring& path)
{
    ShellExecuteW(
        NULL,
        L"open",
        path.c_str(),
        NULL,
        NULL,
        SW_SHOWNORMAL
    );
}


// Create output file path
wstring OutputPath(
    const wstring& originalPath,
    const wstring& fileName)
{
    size_t position = originalPath.find_last_of(L"\\/");

    wstring folder = originalPath.substr(0, position + 1);

    return folder + fileName;
}


// Main
int main()
{
    // Start GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    if (GdiplusStartup(
        &gdiplusToken,
        &gdiplusStartupInput,
        NULL) != Ok)
    {
        cout << "GDI+ initialization failed!\n";
        return 1;
    }

    cout << "=============================\n";
    cout << " IMAGE PROCESSING PROJECT\n";
    cout << "=============================\n";

    // Select image
    wstring imagePath = ChooseImage();

    if (imagePath.empty()) {
        cout << "No image selected!\n";
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    int width, height;

    // Read image
    PixelMatrix original =
        ReadImageMatrix(imagePath, width, height);

    if (width <= 0 || height <= 0) {
        cout << "Invalid image!\n";
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    cout << "\nImage loaded successfully!\n";
    cout << "Width: " << width << endl;
    cout << "Height: " << height << endl;

    cout << "\nOpening original image...\n";
    DisplayImage(imagePath);

    int choice;

    do {
        cout << "\n\n========== MENU ==========\n";
        cout << "1. Grayscale\n";
        cout << "2. Increase Brightness\n";
        cout << "3. Decrease Brightness\n";
        cout << "4. Horizontal Flip\n";
        cout << "5. Black and White\n";
        cout << "0. Exit\n";
        cout << "==========================\n";

        cout << "Enter your choice: ";
        cin >> choice;

        PixelMatrix result;
        wstring outputPath;

        switch (choice) {

        case 1:
            result = Grayscale(original, height, width);
            outputPath = OutputPath(
                imagePath,
                L"Grayscale.jpg"
            );
            break;

        case 2:
            result = Brightness(
                original, height, width, 40
            );
            outputPath = OutputPath(
                imagePath,
                L"Brightness_Increased.jpg"
            );
            break;

        case 3:
            result = Brightness(
                original, height, width, -40
            );
            outputPath = OutputPath(
                imagePath,
                L"Brightness_Decreased.jpg"
            );
            break;

        case 4:
            result = HorizontalFlip(
                original, height, width
            );
            outputPath = OutputPath(
                imagePath,
                L"Horizontal_Flip.jpg"
            );
            break;

        case 5:
            result = BlackWhite(
                original, height, width
            );
            outputPath = OutputPath(
                imagePath,
                L"Black_White.jpg"
            );
            break;

        case 0:
            cout << "Exiting program...\n";
            break;

        default:
            cout << "Invalid choice! Try again.\n";
            continue;
        }

        if (choice >= 1 && choice <= 5) {

            if (SaveImage(
                result,
                width,
                height,
                outputPath))
            {
                cout << "\nImage saved successfully!\n";
                cout << "Opening processed image...\n";

                DisplayImage(outputPath);
            }
            else {
                cout << "\nError saving image!\n";
            }
        }

    } while (choice != 0);

    // Close GDI+
    GdiplusShutdown(gdiplusToken);

    return 0;
}
