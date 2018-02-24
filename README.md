# Image-recognition-dataset-annotation-system-in-Cpp
Image recognition dataset annotation system in C++

The project provides a set of classes for annotation of rectangles on image datasets in order to obtain training datasets for Computer Vision and Machine Learning systems to solve the problems of object detection, image recognition, etc. Some of the classes in the library are:

1. abstract class for getting rectangles from user. The abstract class serves the purpose of a generic annotation code that can in the future make use of any new novel, faster or better ways of annotating rectangles.
1. get rectangles from the user by one click at the top left corner and then drag.
1. get rectangle from user by user inputting two points. There are a few modes:
    1. at two extreme top left and bottom right corners (variable aspect ratio)
    1. one at the center and another at top of the rectangle (assumes fixed aspect ratio)
    1. one at the center and another at right of the rectangle (assumes fixed aspect ratio)
    1. one at the center and another at left of the rectangle (assumes fixed aspect ratio)
    1. one at the center and another at bottom of the rectangle (assumes fixed aspect ratio)
    1. one at the top and another at bottom (assumes fixed aspect ratio)
    1. one at the left and another at the right (assumes fixed aspect ratio)
1. get a rectangle from user with one click at the center of the rectangle. Uses fixed width and height of the rectangle set at the beginning.
1. get rectangle outline.
1. a comprehensive class for manipulating rectangles by the user, i.e. the user  can giving annotation by many different ways.
1. a class that wraps up all of the above for annotating entire datasets. 

There may be pieces of helper functions, header files, etc. that may be missing in the repository.

https://kyaw.xyz/2017/12/18/image-recognition-dataset-annotation-system-cpp

Copyright (C) 2017 Kyaw Kyaw Htike @ Ali Abdul Ghafur. All rights reserved.



Dr. Kyaw Kyaw Htike @ Ali Abdul Ghafur



https://kyaw.xyz
