#!/usr/bin/env python3
import cv2

def main():
    # Load an example image (replace 'sample.jpg' with your image file)
    image = cv2.imread('sample.jpg')
    if image is None:
        print("Image not found. Please provide a valid image file.")
        return

    # Convert the image to grayscale
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Apply Gaussian blur to reduce noise
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    # Run Canny edge detection with thresholds 50 and 150
    edges = cv2.Canny(blurred, 50, 150)

    # Display the original image and the edge map
    cv2.imshow("Original Image", image)
    cv2.imshow("Canny Edges", edges)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
# • This code applies Canny edge detection to an image and displays the results.
# • It uses OpenCV for image processing and edge detection.
# • The image is first converted to grayscale and then blurred to reduce noise.
# • The Canny edge detection algorithm is applied with specified thresholds.
# • The original image and the edge map are displayed in separate windows using OpenCV’s imshow.
# i make TERRIBLE script names
