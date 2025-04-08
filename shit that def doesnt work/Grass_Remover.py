#!/usr/bin/env python3
import cv2
import numpy as np

def main():
    # Load an example image (replace 'sample.jpg' with your image file)
    image = cv2.imread('sample.jpg')
    if image is None:
        print("Image not found. Please provide a valid image file.")
        return

    # Convert the image from BGR to HSV color space
    hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

    # Define lower and upper HSV bounds for green
    lower_green = np.array([35, 40, 40])
    upper_green = np.array([85, 255, 255])

    # Create a mask for green areas
    mask_green = cv2.inRange(hsv, lower_green, upper_green)

    # Invert the mask to obtain non-green regions
    mask_non_green = cv2.bitwise_not(mask_green)

    # Apply the mask to remove green areas from the image
    result = cv2.bitwise_and(image, image, mask=mask_non_green)

    # Display the original and processed images
    cv2.imshow("Original Image", image)
    cv2.imshow("Green Removed", result)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
# • This code removes green areas from an image and displays the result.
# • It uses OpenCV for image processing and color space conversion.
# • The green areas are defined using HSV color space for better accuracy.
# • The processed image is displayed in a window using OpenCV’s imshow.
# • The code is designed to be run as a standalone script, loading an image file and processing it.