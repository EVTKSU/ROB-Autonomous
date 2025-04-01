#!/usr/bin/env python3
import cv2
import numpy as np

def main():
    # Load an example image (replace 'sample_obstacle.jpg' with your image)
    image = cv2.imread('sample_obstacle.jpg')
    if image is None:
        print("Image not found. Please provide a valid image file.")
        return

    # Convert image to grayscale
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    # Apply binary thresholding to isolate objects
    ret, thresh = cv2.threshold(gray, 127, 255, cv2.THRESH_BINARY)
    
    # Find contours in the binary image
    contours, hierarchy = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    # Draw the detected contours on a copy of the original image
    contour_image = image.copy()
    cv2.drawContours(contour_image, contours, -1, (0, 255, 0), 2)
    
    cv2.imshow("Original Image", image)
    cv2.imshow("Detected Contours", contour_image)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
# # • This code detects and draws contours around objects in an image.
# # • It uses OpenCV for image processing and contour detection.
# # • The image is first converted to grayscale and then thresholded to create a binary image.
# # • Contours are found using OpenCV's findContours function.
# # • The detected contours are drawn on a copy of the original image in green.
# # • The original image and the image with detected contours are displayed in separate windows using OpenCV’s imshow.
# # • The code is designed to be run as a standalone script, loading an image file and processing it.
# # • The contours can be used for further analysis or object detection tasks.