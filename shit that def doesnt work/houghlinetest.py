#!/usr/bin/env python3
import cv2
import numpy as np

def main():
    # Load an example image (replace 'sample.jpg' with your image file)
    image = cv2.imread('sample.jpg')
    if image is None:
        print("Image not found. Please provide a valid image file.")
        return

    # Convert to grayscale and blur the image
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    # Detect edges using Canny edge detection
    edges = cv2.Canny(blurred, 50, 150)

    # Apply the Probabilistic Hough Transform to detect lines
    lines = cv2.HoughLinesP(edges, 1, np.pi/180, threshold=50, minLineLength=50, maxLineGap=10)

    # Copy the original image to draw lines on
    line_image = image.copy()
    if lines is not None:
        for line in lines:
            x1, y1, x2, y2 = line[0]
            # Draw each detected line in red
            cv2.line(line_image, (x1, y1), (x2, y2), (0, 0, 255), 2)

    # Display the results
    cv2.imshow("Original Image", image)
    cv2.imshow("Canny Edges", edges)
    cv2.imshow("Hough Lines", line_image)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()

# # • This code applies Canny edge detection and Hough line detection to an image.
# # • It uses OpenCV for image processing and line detection.
# # • The image is first converted to grayscale and blurred to reduce noise.
# # • The Canny edge detection algorithm is applied to find edges in the image.
# # • The Probabilistic Hough Transform is used to detect lines in the edge map.
# # • The detected lines are drawn on a copy of the original image in red.
# # • The original image, edge map, and image with detected lines are displayed in separate windows using OpenCV’s imshow.
# # • The code is designed to be run as a standalone script, loading an image file and processing it.