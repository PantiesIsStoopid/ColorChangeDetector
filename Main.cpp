// Compile Command :

// g++ -o Main Main.cpp -lgdi32 -std=c++23

#include <iostream>
#include <windows.h>
#include <cmath>
#include <chrono>

// Define a struct to hold RGB values
struct RGB
{
  int r;
  int g;
  int b;
};

// Function to calculate the Euclidean distance between two colors
double colorDistance(const RGB &c1, const RGB &c2)
{
  return std::sqrt(std::pow(c1.r - c2.r, 2) + std::pow(c1.g - c2.g, 2) + std::pow(c1.b - c2.b, 2));
}

// Function to capture the current color of the pixel at (x, y)
RGB captureCurrentColour(int x, int y)
{
  HDC hdc = GetDC(NULL);                // Get the device context for the entire screen
  COLORREF color = GetPixel(hdc, x, y); // Get the color of the pixel at (x, y)
  ReleaseDC(NULL, hdc);                 // Release the device context

  RGB rgb;
  rgb.r = GetRValue(color);
  rgb.g = GetGValue(color);
  rgb.b = GetBValue(color);
  return rgb;
}

// Function to get the coordinates of the middle of the screen
void getMiddleScreenCoords(int &x, int &y)
{
  x = GetSystemMetrics(SM_CXSCREEN) / 2;
  y = GetSystemMetrics(SM_CYSCREEN) / 2;
}

// Global variables
HHOOK hHook = NULL;
bool f2Pressed = false;
HWND overlayWindow = NULL;

// Low-level keyboard hook callback function
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode == HC_ACTION)
  {
    KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT *)lParam;
    if (wParam == WM_KEYDOWN && pKeyboard->vkCode == VK_F2)
    {
      f2Pressed = true;
    }
    else if (wParam == WM_KEYUP && pKeyboard->vkCode == VK_F2)
    {
      f2Pressed = false;
    }
  }
  return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// Function to set up the low-level keyboard hook
void SetHook()
{
  hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
}

// Function to remove the low-level keyboard hook
void RemoveHook()
{
  UnhookWindowsHookEx(hHook);
}

// Function to simulate a left mouse click
void SimulateLeftClick()
{
  INPUT inputs[2] = {};

  // Mouse down event
  inputs[0].type = INPUT_MOUSE;
  inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

  // Mouse up event
  inputs[1].type = INPUT_MOUSE;
  inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

  // Send the mouse down event
  SendInput(1, &inputs[0], sizeof(INPUT));
  // Wait for 10 milliseconds
  Sleep(10);
  // Send the mouse up event
  SendInput(1, &inputs[1], sizeof(INPUT));
}

// Function to create an always-on-top overlay window
HWND CreateOverlayWindow()
{
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = DefWindowProcW;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszClassName = L"OverlayWindow";

  RegisterClassW(&wc);

  HWND hwnd = CreateWindowExW(
      WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
      wc.lpszClassName,
      L"Overlay",
      WS_POPUP,
      0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
      NULL, NULL, wc.hInstance, NULL);

  SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);

  ShowWindow(hwnd, SW_SHOW);
  return hwnd;
}

// Function to draw a hollow red box on the overlay window
void DrawHollowRedBox(HWND hwnd, int x, int y, int width, int height)
{
  HDC hdc = GetDC(hwnd);
  HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
  RECT rect = {x, y, x + width, y + height};
  FrameRect(hdc, &rect, brush);
  DeleteObject(brush);
  ReleaseDC(hwnd, hdc);
}

int main()
{
  int x, y;
  getMiddleScreenCoords(x, y);

  RGB previousColor = captureCurrentColour(x, y);
  std::cout << "Initial color at (" << x << ", " << y << ") - R: " << previousColor.r << ", G: " << previousColor.g << ", B: " << previousColor.b << std::endl;

  const double threshold = 10.0; // Threshold for color change detection

  // Set the hook
  SetHook();

  // Create the overlay window
  overlayWindow = CreateOverlayWindow();

  // Message loop to keep the hook and overlay active
  MSG msg;
  while (true)
  {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (f2Pressed)
    {                                                         // Check if F2 key is held down
      auto start = std::chrono::high_resolution_clock::now(); // Start time

      RGB currentColor = captureCurrentColour(x, y);
      double distance = colorDistance(currentColor, previousColor);
      if (distance > threshold)
      {
        SimulateLeftClick();
        std::cout << "Color changed at (" << x << ", " << y << ") - R: " << currentColor.r << ", G: " << currentColor.g << ", B: " << currentColor.b << std::endl;
        previousColor = currentColor; // Update the previous color to the current color
      }
      else
      {
        std::cout << "Color has not changed significantly." << std::endl;
      }

      auto end = std::chrono::high_resolution_clock::now();            // End time
      std::chrono::duration<double, std::milli> elapsed = end - start; // Calculate elapsed time
      std::cout << "Color check took " << elapsed.count() << " ms" << std::endl;
    }

    // Draw the hollow red box on the overlay window
    DrawHollowRedBox(overlayWindow, x - 50, y - 50, 100, 100);

    Sleep(10); // Wait for 10 milliseconds before checking again
  }

  // Remove the hook
  RemoveHook();

  return 0;
}