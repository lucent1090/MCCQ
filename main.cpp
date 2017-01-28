#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>

using namespace cv;
using namespace std;

struct pixel {
	int bgr[3];
};
struct box {
	int start, end;
	int volume, longest_dim;

	bool operator < (const box& b) const {
		return (volume < b.volume);
	}
};

bool cmpB(const pixel& i, const pixel& j) {return (i.bgr[0] < j.bgr[0]);}
bool cmpG(const pixel& i, const pixel& j) {return (i.bgr[1] < j.bgr[1]);}
bool cmpR(const pixel& i, const pixel& j) {return (i.bgr[2] < j.bgr[2]);}
bool (*cmp[3])(const pixel&, const pixel&) = {cmpB, cmpG, cmpR};

vector<pixel> pixels;

void getBoundry(box& b){
	int max[3] = {0, 0, 0}; // [b, g, r]
	int min[3] = {255, 255, 255};
	int max_num = 0, dim = 0, volume = 0;

	for(int i=b.start; i<b.end; i++){
		if(pixels.at(i).bgr[0] > max[0]){ max[0] = pixels.at(i).bgr[0]; }
		if(pixels.at(i).bgr[0] < min[0]){ min[0] = pixels.at(i).bgr[0]; }
	
		if(pixels.at(i).bgr[1] > max[1]){ max[1] = pixels.at(i).bgr[1]; }
		if(pixels.at(i).bgr[1] < min[1]){ min[1] = pixels.at(i).bgr[1]; }
	
		if(pixels.at(i).bgr[2] > max[2]){ max[2] = pixels.at(i).bgr[2]; }
		if(pixels.at(i).bgr[2] < min[2]){ min[2] = pixels.at(i).bgr[2]; }
	}

	for(int i=0; i<3; i++){
		if( (max[i]-min[i]) > max_num ){
			max_num = (max[i] - min[i]);
			dim = i;
		}
	}
	volume = (max[0]-min[0])*(max[1]-min[1])*(max[2]-min[2]);

	b.longest_dim = dim;
	b.volume = volume;
}

int main(int argc, char** argv){
	char* filename = argv[1];
	Mat image = imread(filename, 1);
	if( ! image.data ){
		cout << "Can't open or find the image" << endl;
		return -1;
	}

	for(int i=0; i<image.rows; ++i)
	{
		Vec3b* point = image.ptr<Vec3b>(i);
		for(int j=0; j<image.cols; ++j)
		{
			pixel p;
			p.bgr[0] = (int)point[j][0];
			p.bgr[1] = (int)point[j][1];
			p.bgr[2] = (int)point[j][2];

			pixels.push_back(p);
		}
	}

	priority_queue<box> boxes;

	// initial box
	box init;
	init.start = 0;
	init.end = pixels.size();
	boxes.push(init);

	for(int i=0; i<3; i++)
	{
		box parent, box1, box2;
		
		parent = boxes.top();
		boxes.pop();

		// 1. find boundry, get longest dimension
		getBoundry( parent );

		// 2. find the median
		sort(pixels.begin()+parent.start, pixels.begin()+parent.end, cmp[parent.longest_dim]);
		int median_index = (int)((parent.end - parent.start + 1) / 2);

		// 3. divide into 2 boxes
		box1.start = parent.start;
		box1.end = parent.start + median_index;
		box2.start = parent.start + median_index;
		box2.end = parent.end;

		if( (box1.start > box1.end) || (box2.start > box2.end)){
			cout << "median error!" << endl;
			return -1;
		}
		
		boxes.push(box1);
		boxes.push(box2);
	}

	// calculate average color in each box
	// then draw all average colors
	Mat result_img(50*boxes.size(), 300, CV_8UC3, Scalar(0, 0, 0));

	int boxSize = boxes.size();
	for(int bNum=0; bNum<boxSize; bNum++)
	{
		box drawBox = boxes.top();

		float drawBGR[3] = {0.0, 0.0, 0.0};
		int nPixel = drawBox.end - drawBox.start;

		for(int i=0; i<nPixel; i++){
			drawBGR[0] += pixels.at(drawBox.start+i).bgr[0];
			drawBGR[1] += pixels.at(drawBox.start+i).bgr[1];
			drawBGR[2] += pixels.at(drawBox.start+i).bgr[2];
		}

		drawBGR[0] /= nPixel;
		drawBGR[1] /= nPixel;
		drawBGR[2] /= nPixel;

		for(int i=0; i<50; i++){
			for(int j=0; j<300; j++){
				result_img.at<Vec3b>(bNum*50+i, j).val[0] = (uchar)drawBGR[0];
				result_img.at<Vec3b>(bNum*50+i, j).val[1] = (uchar)drawBGR[1];
				result_img.at<Vec3b>(bNum*50+i, j).val[2] = (uchar)drawBGR[2];
			}
		}

		boxes.pop();
	}

	imwrite("./img/result.png", result_img);
	// namedWindow("result", CV_WINDOW_AUTOSIZE);
	// imshow("result", result_img);

	// waitKey(0);

	return 0;
}