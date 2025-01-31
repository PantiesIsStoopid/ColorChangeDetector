// Compile Command :

// g++ -o Main Main.cpp -lgdi32 -std=c++23

#include <iostream>
#include <windows.h>
#include <cmath>
#include <chrono>

// Define a struct to hold RGB values
struct RGB
{
  int R;
  int G;
  int B;
};

// Function to calculate the Euclidean distance between two colors
double ColorDistance(const RGB &C1, const RGB &C2)
{
  return std::sqrt(std::pow(C1.R - C2.R, 2) + std::pow(C1.G - C2.G, 2) + std::pow(C1.B - C2.B, 2));
}

// Function to capture the current color of the pixel at (x, y)
RGB CaptureCurrentColour(int X, int Y)
{
  HDC Hdc = GetDC(NULL);                // Get the device context for the entire screen
  COLORREF Color = GetPixel(Hdc, X, Y); // Get the color of the pixel at (x, y)
  ReleaseDC(NULL, Hdc);                 // Release the device context

  RGB Rgb;
  Rgb.R = GetRValue(Color);
  Rgb.G = GetGValue(Color);
  Rgb.B = GetBValue(Color);
  return Rgb;
}

// Function to get the coordinates of the middle of the screen
void GetMiddleScreenCoords(int &X, int &Y)
{
  X = GetSystemMetrics(SM_CXSCREEN) / 2;
  Y = GetSystemMetrics(SM_CYSCREEN) / 2;
}

// Global variables
HHOOK HHook = NULL;
bool F2Pressed = false;
HWND OverlayWindow = NULL;

// Low-level keyboard hook callback function
LRESULT CALLBACK LowLevelKeyboardProc(int NCode, WPARAM WParam, LPARAM LParam)
{
  if (NCode == HC_ACTION)
  {
    KBDLLHOOKSTRUCT *PKeyboard = (KBDLLHOOKSTRUCT *)LParam;
    if (WParam == WM_KEYDOWN && PKeyboard->vkCode == VK_F2)
    {
      F2Pressed = true;
    }
    else if (WParam == WM_KEYUP && PKeyboard->vkCode == VK_F2)
    {
      F2Pressed = false;
    }
  }
  return CallNextHookEx(HHook, NCode, WParam, LParam);
}

// Function to set up the low-level keyboard hook
void SetHook()
{
  HHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
}

// Function to remove the low-level keyboard hook
void RemoveHook()
{
  UnhookWindowsHookEx(HHook);
}

// Function to simulate a left mouse click
void SimulateLeftClick()
{
  INPUT Inputs[2] = {};

  // Mouse down event
  Inputs[0].type = INPUT_MOUSE;
  Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

  // Mouse up event
  Inputs[1].type = INPUT_MOUSE;
  Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

  // Send the mouse down event
  SendInput(1, &Inputs[0], sizeof(INPUT));
  // Wait for 10 milliseconds
  Sleep(10);
  // Send the mouse up event
  SendInput(1, &Inputs[1], sizeof(INPUT));
}

// Function to create an always-on-top overlay window
HWND CreateOverlayWindow()
{
  WNDCLASSW Wc = {0};
  Wc.lpfnWndProc = DefWindowProcW;
  Wc.hInstance = GetModuleHandle(NULL);
  Wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  Wc.lpszClassName = L"OverlayWindow";

  RegisterClassW(&Wc);

  HWND Hwnd = CreateWindowExW(
      WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
      Wc.lpszClassName,
      L"Overlay",
      WS_POPUP,
      0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
      NULL, NULL, Wc.hInstance, NULL);

  SetLayeredWindowAttributes(Hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);

  ShowWindow(Hwnd, SW_SHOW);
  return Hwnd;
}

// Function to draw a hollow red box on the overlay window
void DrawHollowRedBox(HWND Hwnd, int X, int Y, int Width, int Height)
{
  HDC Hdc = GetDC(Hwnd);
  HBRUSH Brush = CreateSolidBrush(RGB(255, 0, 0));
  RECT Rect = {X, Y, X + Width, Y + Height};
  FrameRect(Hdc, &Rect, Brush);
  DeleteObject(Brush);
  ReleaseDC(Hwnd, Hdc);
}

int main()
{
  int X, Y;
  GetMiddleScreenCoords(X, Y);

  RGB PreviousColor = CaptureCurrentColour(X, Y);
  std::cout << "Initial color at (" << X << ", " << Y << ") - R: " << PreviousColor.R << ", G: " << PreviousColor.G << ", B: " << PreviousColor.B << std::endl;

  const double Threshold = 10.0; // Threshold for color change detection

  // Set the hook
  SetHook();

  // Create the overlay window
  OverlayWindow = CreateOverlayWindow();

  // Message loop to keep the hook and overlay active
  MSG Msg;
  while (true)
  {
    while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }

    if (F2Pressed)
    {                                                         // Check if F2 key is held down
      auto Start = std::chrono::high_resolution_clock::now(); // Start time

      RGB CurrentColor = CaptureCurrentColour(X, Y);
      double Distance = ColorDistance(CurrentColor, PreviousColor);
      if (Distance > Threshold)
      {
        SimulateLeftClick();
        std::cout << "Color changed at (" << X << ", " << Y << ") - R: " << CurrentColor.R << ", G: " << CurrentColor.G << ", B: " << CurrentColor.B << std::endl;
        PreviousColor = CurrentColor; // Update the previous color to the current color
      }
      else
      {
        std::cout << "Color has not changed significantly." << std::endl;
      }

      auto End = std::chrono::high_resolution_clock::now();            // End time
      std::chrono::duration<double, std::milli> Elapsed = End - Start; // Calculate elapsed time
      std::cout << "Color check took " << Elapsed.count() << " ms" << std::endl;
    }

    // Draw the hollow red box on the overlay window
    DrawHollowRedBox(OverlayWindow, X - 50, Y - 50, 100, 100);

    Sleep(10); // Wait for 10 milliseconds before checking again
  }

  // Remove the hook
  RemoveHook();

  return 0;
}