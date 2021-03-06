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
	int nPixel, volume, longest_dim;

	bool operator < (const box& b) const {
		// The priority of the queue can be number of pixels in the box
		// or the volume of the box.
		// We use number of pixel here
		return (nPixel < b.nPixel);
	}
};

bool cmpB(const pixel& i, const pixel& j) {return (i.bgr[0] < j.bgr[0]);}
bool cmpG(const pixel& i, const pixel& j) {return (i.bgr[1] < j.bgr[1]);}
bool cmpR(const pixel& i, const pixel& j) {return (i.bgr[2] < j.bgr[2]);}
bool (*cmp[3])(const pixel&, const pixel&) = {cmpB, cmpG, cmpR};

vector<pixel> pixels;

void getBoundry(box& b){
	int maxi[3] = {0, 0, 0}; // [b, g, r]
	int mini[3] = {255, 255, 255};
	int max_num = 0, volume = 0;

	pixel p;
	for(int i=b.start; i<b.end; i++){
		p = pixels.at(i);

		mini[0] = min(mini[0], p.bgr[0]);
		maxi[0] = max(maxi[0], p.bgr[0]);
		mini[1] = min(mini[1], p.bgr[1]);
		maxi[1] = max(maxi[1], p.bgr[1]);
		mini[2] = min(mini[2], p.bgr[2]);
		maxi[2] = max(maxi[2], p.bgr[2]);
	}

	for(int i=0; i<3; i++)
	{
		int length = maxi[i] - mini[i];
		if( length > max_num ){
			max_num = length;
			b.longest_dim = i;
		}
	}
	volume = (maxi[0] - mini[0])*(maxi[1] - mini[1])*(maxi[2] - mini[2]);
	if( volume < 0 ){
		cout << "ERROR: " << b.start << " to " << b.end << " volume < 0";
	}

	b.volume = volume;
	b.nPixel = b.end - b.start;

	//-------------------
	// test:
	// check with simple box, works fine.
	//-------------------
}

void getNewColor(int* color, const box& b){
	double sumColor[] = {0.0, 0.0, 0.0};
	pixel p;
	for(int i=b.start; i<b.end; i++){
		p = pixels.at(i);

		sumColor[0] += (double)p.bgr[0];
		sumColor[1] += (double)p.bgr[1];
		sumColor[2] += (double)p.bgr[2];
	}
	for(int i=0; i<3; i++){
		sumColor[i] /= b.nPixel;
		color[i] = (int)sumColor[i];
	}

	//-------------------
	// test:
	// check with simple box, works fine.
	//-------------------
}

void printPixel(const pixel& p){
	cout << "[" << p.bgr[0] << ", " << p.bgr[1] << ", " << p.bgr[2] << "]" << endl;
}
void printPixels(int start, int end){
	for(int i=start; i<end; i++){
		printPixel( pixels.at(i) );
	}
}

int main(int argc, char** argv){
	if( argc != 3 ){
		cout << "please use command: ./MCCQ [input image] [output name]" << endl;
	}

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

			if( (p.bgr[0]!=0) && (p.bgr[1]!=0) && (p.bgr[2]!=0) ){
				pixels.push_back(p);
			}
		}
	}
	//-------------------
	// test:
	// pixels.size() == image.size()
	//-------------------

	priority_queue<box> boxes;

	// initial box
	box init;
	init.start = 0;
	init.end = pixels.size();
	boxes.push(init);

	for(int i=0; i<50; i++)
	{
		box parent, box1, box2;
		
		parent = boxes.top();
		boxes.pop();

		// 1. find boundry, get longest dimension
		getBoundry( parent );

		// 2. find the median
		sort(pixels.begin()+parent.start, pixels.begin()+parent.end, cmp[parent.longest_dim]);
		int median_index = (int)((parent.end - parent.start + 1) / 2);
		//-------------------
		// test:
		// 1. cmp finction and sort work fine
		// 2. since we dont include parent.end, 
		//    (end-start+1)/2 works better than (start+end)/2
		//-------------------	

		if( parent.nPixel < 10 ){
			continue;
		}

		// 3. divide into 2 boxes
		box1.start = parent.start;
		box1.end = parent.start + median_index;
		box2.start = parent.start + median_index;
		box2.end = parent.end;

		if( (box1.start > box1.end) || (box2.start > box2.end)){
			cout << "median error!" << endl;
			return -1;
		}

		getBoundry(box1);
		getBoundry(box2);

		boxes.push(box1);
		boxes.push(box2);
	}

	// calculate average color in each box
	int nBox = boxes.size();
	for(int i=0; i<nBox; i++)
	{
		box drawBox = boxes.top();
		boxes.pop();

		int color[3] = {0, 0, 0};
		getNewColor(color, drawBox);

		for(int j=0; j<drawBox.nPixel; j++){
			for(int k=0; k<3; k++){
				pixels.at(drawBox.start+j).bgr[k] = color[k];
			}
		}
	}

	// then draw the average colors
	int h = image.size().height, w = image.size().width;
	Mat result_img(h, w, CV_8UC3, Scalar(0, 0, 0));
	for(int row=0; row<h; row++){
		for(int col=0; col<w; col++)
		{
			if( (row*h+col) < pixels.size() ){
				for(int k=0; k<3; k++){
					result_img.at<Vec3b>(row, col).val[k] = (uchar)pixels.at(row*h+col).bgr[k];
				}
			}
			
		}
	}
	imwrite(argv[2], result_img);

	return 0;
}