// Copyright (C) 2017 Kyaw Kyaw Htike @ Ali Abdul Ghafur. All rights reserved.

#include "stdafx.h"
#include "fileIO_helpers.h"
#include "timer_ticToc.h"

using namespace std; // for standard C++ lib

// abstract class for getting rectangles from user
// this is useful for annotation datasets for image
// recognition, object detection, etc.
// The abstract class is so that I can write a generic annotation code
// that can in the future make use of any new novel/faster/better ways
// of annotating rectangles .
class getRect_user
{
public:
	virtual ~getRect_user() {};
	// for a given image, get the rectangles
	virtual std::vector<cv::Rect> get_dr(const cv::Mat &img) = 0;
protected:
};

// get a rectangle by one click at the top left corner
// and then drag.
class getRect_1click_drag : public getRect_user
{
public:

	getRect_1click_drag()
	{
		name_win = "Get rectangles from user";
		thickness_rect = 2;
		color_rect = cv::Scalar(255, 0, 0, 0);
	}

	getRect_1click_drag(std::string name_win_, 
		int thickness_rect_=2, cv::Scalar color_rect_= cv::Scalar(255, 0, 0, 0))
	{
		name_win = name_win_;
		thickness_rect = thickness_rect_;
		color_rect = color_rect_;
	}

	std::vector<cv::Rect> get_dr(const cv::Mat &img) override
	{ 
		dr.clear(); dr.reserve(30);
		being_dragged = false;
		img_canvas = img.clone();
		cv::namedWindow(name_win);
		cv::imshow(name_win, img);
		cv::setMouseCallback(name_win, CallBackFunc, this);
		cv::waitKey(0);
		return dr; 
	}

	cv::Mat get_img_drawn() { return img_canvas; }

	//==========================================//
	// Public data members: not for users to call directly; for CallBackFunc static method
	//==========================================//
	std::string name_win;
	int thickness_rect;
	cv::Scalar color_rect;
	std::vector<cv::Rect> dr;
	cv::Mat img_canvas;
	bool being_dragged;
	cv::Point point1, point2;

private:

	static void CallBackFunc(int event, int x, int y, int flags, void* userdata)
	{
		getRect_1click_drag* thisObj = static_cast<getRect_1click_drag*>(userdata);

		if (event == CV_EVENT_LBUTTONDOWN && !thisObj->being_dragged)
		{
			/* left button clicked. ROI selection begins */
			thisObj->point1 = cv::Point(x, y);
			thisObj->being_dragged = true;
		}

		if (event == CV_EVENT_MOUSEMOVE && thisObj->being_dragged)
		{
			/* mouse dragged. ROI being selected */
			cv::Mat img_temp = thisObj->img_canvas.clone();
			thisObj->point2 = cv::Point(x, y);
			cv::rectangle(img_temp, thisObj->point1, thisObj->point2, thisObj->color_rect, thisObj->thickness_rect);
			cv::imshow(thisObj->name_win, img_temp);
		}

		if (event == CV_EVENT_LBUTTONUP && thisObj->being_dragged)
		{
			thisObj->point2 = cv::Point(x, y);
			thisObj->being_dragged = false;
			cv::rectangle(thisObj->img_canvas, thisObj->point1, thisObj->point2, thisObj->color_rect, thisObj->thickness_rect);
			cv::imshow(thisObj->name_win, thisObj->img_canvas);
			thisObj->dr.push_back(cv::Rect(thisObj->point1, thisObj->point2));			
		}
	}
};

// get rectangle from user by user inputting two points. There are a few
// modes:
// (1) at two extreme top left and bottom right corners (variable aspect ratio) 
// (2) one at the center and another at top of the rectangle (assumes fixed aspect ratio)
// (3) one at the center and another at right of the rectangle (assumes fixed aspect ratio)
// (4) one at the center and another at left of the rectangle (assumes fixed aspect ratio)
// (5) one at the center and another at bottom of the rectangle (assumes fixed aspect ratio)
// (6) one at the top and another at bottom (assumes fixed aspect ratio)
// (7) one at the left and another at the right (assumes fixed aspect ratio)
class getRect_2clicks : public getRect_user
{
public:

	enum ModeClicks
	{
		TL_BR, C_T, C_R, C_L, C_B, T_B, L_R
	};

	getRect_2clicks()
	{
		mode_click = ModeClicks::TL_BR;
		aspect_ratio = 0.5; // width / height
		name_win = "Get rectangles from user";
		thickness_rect = 2;
		color_rect = cv::Scalar(255, 0, 0, 0);		
	}

	// for ModeClicks::TL_BR, aspect_ratio_ is being 0 has a special meaning that
	// no aspect ratio constraint is to be used. Meaning the two extreme corners annotated
	// will be the exact two extreme corners of the rectangle. If however the aspect ratio
	// is not zero, then from the rectangle, it will change the height or width so that
	// the aspect ratio is the same as the given aspect ratio value. If aspect ratio is
	// a positive value, then it will keep the height and adjust the width; if the
	// aspect ratio is negative, then abs. val of the aspect ratio is taken and 
	// keep the width and adjust the height.
	getRect_2clicks(float aspect_ratio_, ModeClicks mode_click_ = ModeClicks::TL_BR, std::string name_win_ = "Get rectangles from user",
		int thickness_rect_ = 2, cv::Scalar color_rect_ = cv::Scalar(255, 0, 0, 0))
	{
		mode_click = mode_click_;
		aspect_ratio = aspect_ratio_;
		name_win = name_win_;
		thickness_rect = thickness_rect_;
		color_rect = color_rect_;		
	}

	std::vector<cv::Rect> get_dr(const cv::Mat &img)  override
	{ 
		firstClickDone = false;
		img_canvas = img.clone();
		dr.clear(); dr.reserve(30);
		cv::namedWindow(name_win);
		cv::imshow(name_win, img);
		cv::setMouseCallback(name_win, CallBackFunc, this);
		cv::waitKey(0);
		return dr; 
	}

	cv::Mat get_img_drawn() { return img_canvas; }

	//==========================================//
	// Public data members: not for users to call directly; for CallBackFunc static method
	//==========================================//
	std::string name_win;
	int thickness_rect;
	cv::Scalar color_rect;
	std::vector<cv::Rect> dr;
	cv::Mat img_canvas;
	cv::Point point1, point2;
	bool firstClickDone;
	ModeClicks mode_click;
	float aspect_ratio;

private:

	static void CallBackFunc(int event, int x, int y, int flags, void* userdata)
	{
		getRect_2clicks* thisObj = static_cast<getRect_2clicks*>(userdata);
		
		if (event == CV_EVENT_LBUTTONUP)
		{
			// process second click: got a rectangle; display & save it
			if (thisObj->firstClickDone) 
			{
				thisObj->point2 = cv::Point(x, y);
				cv::Rect rect_cur;
				
				switch (thisObj->mode_click)
				{
				case getRect_2clicks::ModeClicks::TL_BR:
				{
					rect_cur = cv::Rect(thisObj->point1, thisObj->point2);

					// if not the special user specified aspect ratio of 0
					// then I will adjust the rect_cur so that it will
					// satisfy the abs(aspect_ratio)
					if (thisObj->aspect_ratio != 0)
					{
						// estimate center of rectangle
						int cx = thisObj->point1.x + rect_cur.width / 2;
						int cy = thisObj->point1.y + rect_cur.height / 2;
						int w, h;
						
						// keep height and change width
						if (thisObj->aspect_ratio > 0)
						{
							h = rect_cur.height;
							w = static_cast<float>(h) * std::abs(thisObj->aspect_ratio);
						}
						// keep width, change height
						else
						{
							w = rect_cur.width;
							h = static_cast<float>(w) / std::abs(thisObj->aspect_ratio);
						}
						rect_cur = cv::Rect(cx - w / 2, cy - h / 2, w, h);
					}
					break;
				}

				case getRect_2clicks::ModeClicks::C_T:
				{
					// find the half height first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int h = rect_temp.height * 2;
					int w = std::round(thisObj->aspect_ratio * static_cast<float>(h));
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case getRect_2clicks::ModeClicks::C_R:
				{
					// find half width first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int w = rect_temp.width * 2;
					int h = std::round(static_cast<float>(w) / thisObj->aspect_ratio);
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case getRect_2clicks::ModeClicks::C_L:
				{
					// find half width first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int w = rect_temp.width * 2;
					int h = std::round(static_cast<float>(w) / thisObj->aspect_ratio);
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case getRect_2clicks::ModeClicks::C_B:
				{
					// find the half height first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int h = rect_temp.height * 2;
					int w = std::round(thisObj->aspect_ratio * static_cast<float>(h));
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case getRect_2clicks::ModeClicks::T_B:
				{
					// find the full height first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int h = rect_temp.height;
					int w = std::round(thisObj->aspect_ratio * static_cast<float>(h));
					rect_cur = cv::Rect((rect_temp.x + rect_temp.width / 2) - w / 2,
						(rect_temp.y + rect_temp.height / 2) - h / 2, w, h);
					break;
				}

				case getRect_2clicks::ModeClicks::L_R:
				{
					// find the full width first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int w = rect_temp.width;
					int h = std::round(static_cast<float>(w) / thisObj->aspect_ratio);
					rect_cur = cv::Rect((rect_temp.x + rect_temp.width / 2) - w / 2,
						(rect_temp.y + rect_temp.height / 2) - h / 2, w, h);
					break;
				}

				} // end switch		

				cv::rectangle(thisObj->img_canvas, rect_cur, thisObj->color_rect, thisObj->thickness_rect);
				cv::imshow(thisObj->name_win, thisObj->img_canvas);
				thisObj->dr.push_back(rect_cur);
				thisObj->firstClickDone = false;
			}

			// process first click; just display the current image + the initial point
			// wait for the second click to form the rectangle
			else
			{
				thisObj->point1 = cv::Point(x, y);
				cv::Mat img_temp = thisObj->img_canvas.clone();
				cv::drawMarker(img_temp, thisObj->point1, thisObj->color_rect, 0, 20, 2, 8);
				cv::imshow(thisObj->name_win, img_temp);
				thisObj->firstClickDone = true;
			}			
		}
	}
};

// get a rectangle from user with one click at the center of the rectangle
// Uses fixed width and height of the rectangle set at the beginning.
class getRect_1click : public getRect_user
{
public:

	getRect_1click() = delete;

	// this is the main constructor to use;
	// don't use the other constructors; that's only for
	// marking with crosses, etc. for other purposes
	getRect_1click(const cv::Size &rectSize_,
		std::string name_win_ = "Get rectangles from user",
		int thickness_rect_ = 2, 
		cv::Scalar color_rect_ = cv::Scalar(255, 0, 0, 0))
	{
		name_win = name_win_;
		rectSize = rectSize_;
		thickness = thickness_rect_;
		color = color_rect_;
		draw_rect_mode = true;
	}

	// display fixed size cross mark with marked point version
	getRect_1click(std::string name_win_ = "Get rectangles from user",
		int markerType_= cv::MARKER_CROSS,
		int thickness_marker_ = 2, cv::Scalar color_marker_ = cv::Scalar(255, 0, 0, 0), 
		int markerSize_=20, int lineType_=8)
	{
		name_win = name_win_;
		markerType = markerType_;
		thickness = thickness_marker_;
		color = color_marker_;
		markerSize = markerSize_;
		lineType = lineType_;
		rectSize = cv::Size(8, 8); // just some dummy value
		draw_rect_mode = false;			
	}

	std::vector<cv::Rect> get_dr(const cv::Mat &img)  override
	{ 
		img_canvas = img.clone();
		dr.clear(); dr.reserve(30);
		points_marked.clear(); points_marked.reserve(30);

		cv::namedWindow(name_win);
		cv::imshow(name_win, img);
		cv::setMouseCallback(name_win, CallBackFunc, this);
		cv::waitKey(0);
		return dr; 
	}

	std::vector<cv::Point> get_points() { return points_marked; }
	cv::Mat get_img_drawn() { return img_canvas; }

	//==========================================//
	// Public data members: not for users to call directly; for CallBackFunc static method
	//==========================================//

	std::string name_win;
	cv::Mat img_canvas;
	bool draw_rect_mode; // if true, will draw fixed size rectangle, else cross mark
	int thickness;
	cv::Scalar color;
	std::vector<cv::Rect> dr; // recorded rectangles
	std::vector<cv::Point> points_marked; // recorded points

	// for drawing fixed size rectangle with the point
	cv::Size rectSize;

	// for drawing fixed size cross mark with the point
	int markerType;
	int lineType;
	int markerSize;

private:
	
	static void CallBackFunc(int event, int x, int y, int flags, void* userdata)
	{
		getRect_1click* thisObj = static_cast<getRect_1click*>(userdata);

		if (event == CV_EVENT_LBUTTONUP)
		{
			cv::Point point_cur = cv::Point(x, y);
			cv::Rect rect_cur = cv::Rect(point_cur, thisObj->rectSize);
			// rect_cur is such that the point_cur is at its center
			rect_cur -= cv::Point(thisObj->rectSize / 2); 
			if (thisObj->draw_rect_mode)
				cv::rectangle(thisObj->img_canvas, rect_cur, thisObj->color, thisObj->thickness);
			else
				cv::drawMarker(thisObj->img_canvas, point_cur, thisObj->color, thisObj->markerType,
					thisObj->markerSize, thisObj->thickness, thisObj->lineType);
			cv::imshow(thisObj->name_win, thisObj->img_canvas);
			thisObj->points_marked.push_back(point_cur);
			thisObj->dr.push_back(rect_cur);
		}

	}


};

class getRect_outLine : public getRect_user
{
public:

	getRect_outLine() = delete;

	// this is the main constructor to use;
	// don't use the other constructors; that's only for
	// marking with crosses, etc. for other purposes
	getRect_outLine(const cv::Size &rectSize_, double scale_img_ = 1, 
		std::string name_win_ = "Get rectangles from user",
		int thickness_rect_ = 2,
		cv::Scalar color_rect_ = cv::Scalar(255, 0, 0, 0))
	{
		name_win = name_win_;
		rectSize = rectSize_;
		thickness = thickness_rect_;
		color = color_rect_;
		scale_img = scale_img_;

		rectSize.width = std::round(rectSize.width * scale_img);
		rectSize.height = std::round(rectSize.height * scale_img);
	}

	void set_draw_cross_mode(int size_marker_ = 20, int thickness_marker_ = 2, int lineType_marker_ = 8, 
		cv::Scalar color_marker_ = cv::Scalar(255, 0, 0, 0), int type_marker_ = cv::MARKER_CROSS)
	{
		draw_cross_mode = true;
		color_marker = color_marker_;
		type_marker = type_marker_;
		size_marker = size_marker_;
		thickness_marker = thickness_marker_;
		lineType_marker = lineType_marker_;
	}

	std::vector<cv::Rect> get_dr(const cv::Mat &img)  override
	{
		being_dragged = false;
		img_canvas = img.clone();

		cv::resize(img_canvas, img_canvas, cv::Size(), scale_img, scale_img);

		dr.clear(); dr.reserve(1000);

		cv::namedWindow(name_win);
		cv::imshow(name_win, img_canvas);
		cv::setMouseCallback(name_win, CallBackFunc, this);
		cv::waitKey(0);
		return dr;
	}

	cv::Mat get_img_drawn() { return img_canvas; }

	//==========================================//
	// Public members not for users to call directly; for CallBackFunc static method
	//==========================================//

	std::string name_win;
	cv::Mat img_canvas;
	int thickness;
	bool being_dragged;
	cv::Scalar color;
	
	std::vector<cv::Rect> dr; // recorded rectangles
	double scale_img;

	bool draw_cross_mode;
	cv::Scalar color_marker;
	int type_marker;
	int size_marker;
	int thickness_marker;
	int lineType_marker;

	// for drawing fixed size rectangle with the point
	cv::Size rectSize;

	void process_point(const cv::Point &point_cur)
	{
		cv::Rect rect_cur = cv::Rect(point_cur, rectSize);
		cout << "point_cur: " << point_cur << endl;
		cout << "rectSize: " << rectSize << endl;
		rect_cur -= cv::Point(rectSize / 2);
		if (rect_cur.x + rect_cur.width >= img_canvas.cols || 
			rect_cur.y + rect_cur.height >= img_canvas.rows ||
			rect_cur.x < 0 || rect_cur.y < 0)
		{
			cout << "The resulting rectangle is out of image boundary. Ignoring..." << endl;
			return;
		}
		if(draw_cross_mode)
			cv::drawMarker(img_canvas, point_cur, color_marker, type_marker,
				size_marker, thickness_marker, lineType_marker);
		else
			cv::rectangle(img_canvas, rect_cur, color, thickness);
		cv::imshow(name_win, img_canvas);
		rect_cur = cv::Rect(std::round(static_cast<double>(rect_cur.x) / scale_img),
			std::round(static_cast<double>(rect_cur.y) / scale_img),
			std::round(static_cast<double>(rect_cur.width) / scale_img),
			std::round(static_cast<double>(rect_cur.height) / scale_img));	
		cout << "Recording rectangle: " << rect_cur << endl;
		dr.push_back(rect_cur);
	}

private:

	static void CallBackFunc(int event, int x, int y, int flags, void* userdata)
	{
		getRect_outLine* thisObj = static_cast<getRect_outLine*>(userdata);
		
		if (event == CV_EVENT_LBUTTONDOWN && !thisObj->being_dragged)
		{
			thisObj->process_point(cv::Point(x, y));
			thisObj->being_dragged = true;
		}

		if (event == CV_EVENT_MOUSEMOVE && thisObj->being_dragged)
		{
			thisObj->process_point(cv::Point(x, y));
		}

		if (event == CV_EVENT_LBUTTONUP && thisObj->being_dragged)
		{
			thisObj->process_point(cv::Point(x, y));
			thisObj->being_dragged = false;
		}

	}


};
 
class manipRect
{
public:

	enum ModeClicks
	{
		TL_BR, C_T, C_R, C_L, C_B, T_B, L_R
	};

	manipRect()
	{
		mode_click = ModeClicks::TL_BR;
		aspect_ratio = 0.5; // width / height
		name_win = "Get rectangles from user";
		thickness_rect = 2;
		color_rect = cv::Scalar(255, 0, 0, 0);
	}

	// for ModeClicks::TL_BR, aspect_ratio_ is being 0 has a special meaning that
	// no aspect ratio constraint is to be used. Meaning the two extreme corners annotated
	// will be the exact two extreme corners of the rectangle. If however the aspect ratio
	// is not zero, then from the rectangle, it will change the height or width so that
	// the aspect ratio is the same as the given aspect ratio value. If aspect ratio is
	// a positive value, then it will keep the height and adjust the width; if the
	// aspect ratio is negative, then abs. val of the aspect ratio is taken and 
	// keep the width and adjust the height.
	manipRect(float aspect_ratio_, ModeClicks mode_click_ = ModeClicks::TL_BR, std::string name_win_ = "Get rectangles from user",
		int thickness_rect_ = 2, cv::Scalar color_rect_ = cv::Scalar(255, 0, 0, 0))
	{
		mode_click = mode_click_;
		aspect_ratio = aspect_ratio_;
		name_win = name_win_;
		thickness_rect = thickness_rect_;
		color_rect = color_rect_;
	}
	
	std::vector<cv::Rect> get_dr(const cv::Mat &img, const std::vector<cv::Rect> dr_)
	{
		firstClickDone = false;
		img_canvas_orig = img.clone();
		img_canvas = img.clone();
		dr = dr_;
		update_canvas();
		dr.reserve(30);
		cv::namedWindow(name_win);
		cv::imshow(name_win, img_canvas);
		val_trackbar = 0;
		being_dragged = false;
		cv::createTrackbar("Delete mode", name_win, &val_trackbar, 1, CallBackFunc_trackbar, this);
		cv::setMouseCallback(name_win, CallBackFunc_mouse, this);
		cv::waitKey(0);
		return dr;
	}

	// update the image canvas with current latest vector of rectangles
	// this can 
	void update_canvas()
	{
		img_canvas = img_canvas_orig.clone();
		for (size_t i = 0; i < dr.size(); i++)
			cv::rectangle(img_canvas, dr[i], color_rect, thickness_rect);		
	}

	// find the index of the nearest rectangles among the 
	// vector of rectangles (in the data member dr) from the given point p
	int find_nearest_rect(const cv::Point &p)
	{
		std::vector<cv::Rect> dr_temp = dr;
		auto center_rect = [](cv::Rect rr) { return cv::Point(rr.x + rr.width / 2, rr.y + rr.height / 2); };
		auto dist = [&center_rect](cv::Rect rr, cv::Point pp) { return std::sqrt(std::pow(center_rect(rr).x - pp.x, 2) + std::pow(center_rect(rr).y - pp.y, 2)); };
		auto compare = [&p, &dr_temp, &dist](int i, int j) { return dist(dr_temp[i], p) < dist(dr_temp[j], p); };

		std::vector<int> indices(dr.size());
		std::iota(indices.begin(), indices.end(), 0);

		std::nth_element(indices.begin(), indices.begin() + 1, indices.end(), compare);

		return indices[0];
	}

	cv::Mat get_img_drawn() { return img_canvas; }

	//==========================================//
	// Public data members: not for users to call directly; for CallBackFunc static method
	//==========================================//
	std::string name_win;
	int thickness_rect;
	cv::Scalar color_rect;
	std::vector<cv::Rect> dr;
	cv::Mat img_canvas;
	cv::Mat img_canvas_orig; // to save it so that I can use it in case of redraws
	cv::Point point1, point2;
	bool firstClickDone;
	bool being_dragged;
	cv::Rect rect_dragged; // for moving an existing rectangle
	ModeClicks mode_click;
	float aspect_ratio;
	// if 0, then new rectangle or move existing rectangles
	// if 1, delete rectangle mode
	int val_trackbar; 

private:

	/*
	----------------------------
	When val_trackbar == 0 (non-delete mode)
	----------------------------
	(1) two separate left button clicks (not double click): add a new rectangle
	how the rectangle is formed depends on ModeClicks. 
	(2) right button click and drag: move an existing nearest rectangle
	----------------------------
	When val_trackbar == 1 (delete mode)
	----------------------------
	(1) right button click: delete the nearest rectangle
	(2) left button click and drag: form a bounding box in which all rectangles 
	that are fully contained inside it will be deleted.
	*/
	
	static void CallBackFunc_mouse(int event, int x, int y, int flags, void* userdata)
	{
		manipRect* thisObj = static_cast<manipRect*>(userdata);

		// ======================================================= //
		// delete mode: case 1 (deleting single rectangle by single right click)
		// ======================================================= //
		if (event == CV_EVENT_RBUTTONDOWN && thisObj->val_trackbar == 1)
		{
			cv::Point p = cv::Point(x, y);
			thisObj->dr.erase(thisObj->dr.begin() + thisObj->find_nearest_rect(p));
			thisObj->update_canvas();
			cv::imshow(thisObj->name_win, thisObj->img_canvas);
		}
		
		// ======================================================= //
		// delete mode: case 2 (deleting multiple rectangles by bounding box using left click & drag)
		// ======================================================= //
		
		if (event == CV_EVENT_LBUTTONDOWN && !thisObj->being_dragged && thisObj->val_trackbar == 1)
		{
			/* left button clicked. ROI selection begins */
			thisObj->point1 = cv::Point(x, y);
			thisObj->being_dragged = true;
		}

		if (event == CV_EVENT_MOUSEMOVE && thisObj->being_dragged && thisObj->val_trackbar == 1)
		{
			/* mouse dragged. ROI being selected */
			cv::Mat img_temp = thisObj->img_canvas.clone();
			thisObj->point2 = cv::Point(x, y);
			cv::rectangle(img_temp, thisObj->point1, thisObj->point2, thisObj->color_rect, thisObj->thickness_rect);
			cv::imshow(thisObj->name_win, img_temp);
		}

		if (event == CV_EVENT_LBUTTONUP && thisObj->being_dragged && thisObj->val_trackbar == 1)
		{
			thisObj->point2 = cv::Point(x, y);
			thisObj->being_dragged = false;
			// box within which to delete all the rectangles
			cv::Rect rect_delBox(thisObj->point1, thisObj->point2);
			std::vector<cv::Rect>::iterator iter;
			for (iter= thisObj->dr.begin(); iter!= thisObj->dr.end();)
			{
				cv::Point centre_cur_box = cv::Point(iter->x + iter->width / 2, iter->y + iter->height / 2);
				if (rect_delBox.contains(centre_cur_box))
					iter = thisObj->dr.erase(iter);
				else
					++iter;
			}
			thisObj->update_canvas();
			cv::imshow(thisObj->name_win, thisObj->img_canvas);
		}
		
		// ======================================================= //
		// non-delete mode: case 1 (new rectangle)
		// ======================================================= //
		if (event == CV_EVENT_LBUTTONUP && !thisObj->being_dragged && thisObj->val_trackbar == 0)
		{
			// process second click: got a rectangle; display & save it
			if (thisObj->firstClickDone)
			{
				thisObj->point2 = cv::Point(x, y);
				cv::Rect rect_cur;

				switch (thisObj->mode_click)
				{
				case manipRect::ModeClicks::TL_BR:
				{
					rect_cur = cv::Rect(thisObj->point1, thisObj->point2);

					// if not the special user specified aspect ratio of 0
					// then I will adjust the rect_cur so that it will
					// satisfy the abs(aspect_ratio)
					if (thisObj->aspect_ratio != 0)
					{
						// estimate center of rectangle
						int cx = thisObj->point1.x + rect_cur.width / 2;
						int cy = thisObj->point1.y + rect_cur.height / 2;
						int w, h;

						// keep height and change width
						if (thisObj->aspect_ratio > 0)
						{
							h = rect_cur.height;
							w = static_cast<float>(h) * std::abs(thisObj->aspect_ratio);
						}
						// keep width, change height
						else
						{
							w = rect_cur.width;
							h = static_cast<float>(w) / std::abs(thisObj->aspect_ratio);
						}
						rect_cur = cv::Rect(cx - w / 2, cy - h / 2, w, h);
					}
					break;
				}

				case manipRect::ModeClicks::C_T:
				{
					// find the half height first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int h = rect_temp.height * 2;
					int w = std::round(thisObj->aspect_ratio * static_cast<float>(h));
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case manipRect::ModeClicks::C_R:
				{
					// find half width first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int w = rect_temp.width * 2;
					int h = std::round(static_cast<float>(w) / thisObj->aspect_ratio);
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case manipRect::ModeClicks::C_L:
				{
					// find half width first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int w = rect_temp.width * 2;
					int h = std::round(static_cast<float>(w) / thisObj->aspect_ratio);
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case manipRect::ModeClicks::C_B:
				{
					// find the half height first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int h = rect_temp.height * 2;
					int w = std::round(thisObj->aspect_ratio * static_cast<float>(h));
					rect_cur = cv::Rect(thisObj->point1.x - w / 2, thisObj->point1.y - h / 2, w, h);
					break;
				}

				case manipRect::ModeClicks::T_B:
				{
					// find the full height first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int h = rect_temp.height;
					int w = std::round(thisObj->aspect_ratio * static_cast<float>(h));
					rect_cur = cv::Rect((rect_temp.x + rect_temp.width / 2) - w / 2,
						(rect_temp.y + rect_temp.height / 2) - h / 2, w, h);
					break;
				}

				case manipRect::ModeClicks::L_R:
				{
					// find the full width first
					cv::Rect rect_temp(thisObj->point1, thisObj->point2);
					int w = rect_temp.width;
					int h = std::round(static_cast<float>(w) / thisObj->aspect_ratio);
					rect_cur = cv::Rect((rect_temp.x + rect_temp.width / 2) - w / 2,
						(rect_temp.y + rect_temp.height / 2) - h / 2, w, h);
					break;
				}

				} // end switch		

				cv::rectangle(thisObj->img_canvas, rect_cur, thisObj->color_rect, thisObj->thickness_rect);
				cv::imshow(thisObj->name_win, thisObj->img_canvas);
				thisObj->dr.push_back(rect_cur);
				thisObj->firstClickDone = false;
			}

			// process first click; just display the current image + the initial point
			// wait for the second click to form the rectangle
			else
			{
				thisObj->point1 = cv::Point(x, y);
				cv::Mat img_temp = thisObj->img_canvas.clone();
				cv::drawMarker(img_temp, thisObj->point1, thisObj->color_rect, 0, 20, 2, 8);
				cv::imshow(thisObj->name_win, img_temp);
				thisObj->firstClickDone = true;
			}
		}

		// ======================================================= //
		// non-delete mode: case 2 (moving using right click & drag)
		// ======================================================= //
		if (event == CV_EVENT_RBUTTONDOWN && !thisObj->being_dragged && thisObj->val_trackbar == 0)
		{
			thisObj->point1 = cv::Point(x, y);
			thisObj->being_dragged = true;
			int idx_rect_sel = thisObj->find_nearest_rect(thisObj->point1);
			thisObj->rect_dragged = thisObj->dr[idx_rect_sel];
			thisObj->dr.erase(thisObj->dr.begin()+ idx_rect_sel);
			thisObj->update_canvas();
		}

		if (event == CV_EVENT_MOUSEMOVE && thisObj->being_dragged && thisObj->val_trackbar == 0)
		{
			cv::Mat img_temp = thisObj->img_canvas.clone();
			cv::Point p = cv::Point(x, y);
			cv::Rect rec_cur(p.x - thisObj->rect_dragged.width / 2, p.y - thisObj->rect_dragged.height / 2,
				thisObj->rect_dragged.width, thisObj->rect_dragged.height);
			cv::rectangle(img_temp, rec_cur, thisObj->color_rect, thisObj->thickness_rect);
			cv::imshow(thisObj->name_win, img_temp);
		}

		if (event == CV_EVENT_RBUTTONUP && thisObj->being_dragged && thisObj->val_trackbar == 0)
		{
			cv::Point p = cv::Point(x, y);
			thisObj->being_dragged = false;
			cv::Rect rec_cur(p.x - thisObj->rect_dragged.width / 2, p.y - thisObj->rect_dragged.height / 2,
				thisObj->rect_dragged.width, thisObj->rect_dragged.height);
			thisObj->dr.push_back(rec_cur);			
			thisObj->update_canvas();
			cv::imshow(thisObj->name_win, thisObj->img_canvas);
		}

	}

	static void CallBackFunc_trackbar(int pos, void* userdata)
	{
		manipRect* thisObj = static_cast<manipRect*>(userdata);
		thisObj->val_trackbar = pos;
	}

};


// annotate object detection dataset
class annotate_obj_det_dataset
{
private:
	cv::Size winsize; // detection window size
	std::string dir_images, dir_output;
	getRect_user &getRect_obj;

public:

	// make sure that "dir_images_" has "/" at the end
	annotate_obj_det_dataset(std::string dir_images_, std::string dir_output_, 
		const cv::Size &winsize_, getRect_user &getRect_obj_)
		: getRect_obj(getRect_obj_)
	{
		dir_images = dir_images_;
		dir_output = dir_output_;
		winsize = winsize_;		

		if (dir_images[dir_images.size() - 1] != '/')
		{
			printf("ERROR: dir_images_ must end with '/'\n");
			throw std::runtime_error("");
		}

		if (dir_output[dir_output.size() - 1] != '/')
		{
			printf("ERROR: dir_output_ must end with '/'\n");
			throw std::runtime_error("");
		}

	}

	// extract patches from a given image and vector of rectangles
	std::vector<cv::Mat> extract_patches(const cv::Mat &img, const std::vector<cv::Rect> &recs)
	{
		std::vector<cv::Mat> patches(recs.size());
		for (int i = 0; i < recs.size(); i++)
			patches[i] = img(recs[i]);
		return patches;
	}

	void annotate()
	{
		// read in image full paths
		std::vector<std::string> str_exts = { "*.png", "*.jpg", "*.jpeg", "*.tif", "*.tiff" };
		std::vector<std::string> fpaths;
		dir_fnames(dir_images, str_exts, fpaths);
		cout << "Number of images to annotate = " << fpaths.size() << endl;

		cv::Mat img;
		std::vector<cv::Mat> patches;
		std::vector<cv::Rect> dr;
		std::string fname_out;
		int counter = 0;

		// go through each image and annotate with bounding boxes
		for (size_t i = 0; i < fpaths.size(); i++)
		{
			cout << "Annotating image: " << fpaths[i] << endl;
			img = cv::imread(fpaths[i]);
			dr = getRect_obj.get_dr(img);
			patches = extract_patches(img, dr);
			cout << "Obtained " << patches.size() << " patches." << endl;
			for (size_t j = 0; j < patches.size(); j++)
			{
				counter++;
				fname_out = fmt::sprintf("%s%05d.png", dir_output, counter);
				//cv::resize(patches[j], patches[j], winsize);
				cv::imwrite(fname_out, patches[j]);
			}
		}

	
	} // end method "annotate"

};




int main(int argc, char* argv[])
{	

	//cv::Mat img9 = cv::imread("D:/Research/Datasets/INRIAPerson_Piotr/Test/images/set01/V000/I00000.png");
	//std::vector<cv::Rect> dr9 = getRect_1click_drag(img9).get_dr();
	//cv::Mat img10 = getRect_1click_drag(img9).get_img_drawn();
	//std::vector<cv::Mat> p = cv::extract_patches(img9, dr9);
	
	//imshow("win1", p[0]);
	//imshow("win2", p[1]);
	//imshow("win3", p[2]);
	//cv::waitKey(0);

	//cv::Mat img99 = getPoint_user(img9, "w").get_img_drawn();
	//cv::imshow("2", img99); cv::waitKey(0);

	//getRect_1click2 gr(cv::Size(64,64));
	//std::vector<cv::Rect> dr9 = gr.get_dr(img9);
	//imshow_dr("win", img9, dr9);

	//manipRect gr(0, manipRect::TL_BR, "win1");
	//std::vector<cv::Rect> dr9 = gr.get_dr(img9, std::vector<cv::Rect>());
	//dr9.push_back(dr9[0] + cv::Point(64, 64));
	//std::vector<cv::Rect> dr10 = gr.get_dr(img9, dr9);
	////imshow_dr("win", img9, dr10);

	////std::string dir_images = "D:/Research/Datasets/INRIAPerson_Piotr/Test/images/set01/V000/";
	//std::string dir_images = "D:/Research/Datasets/INRIAPerson_Piotr/Train/imgs_crop_context/";
	//std::string dir_output = "C:/Users/Kyaw/Desktop/train_imgs_pos/";
	//cv::Size winsize(64, 128);
	////getRect_2clicks gr(0.5, getRect_2clicks::C_T);
	////getRect_1click gr(cv::Size(16,16));
	//getRect_outLine gr(cv::Size(16, 16), 4);
	//gr.set_draw_cross_mode(5);

	//annotate_obj_det_dataset gann(dir_images, dir_output, winsize, gr);
	//gann.annotate();
	//
	//return 1;
	
}