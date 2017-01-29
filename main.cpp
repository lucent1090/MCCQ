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

	for(int i=b.start; i<b.end; i++){
		for(int j=0; j<3; j++){
			mini[j] = min(mini[j], pixels.at(i).bgr[j]);
			maxi[j] = max(maxi[j], pixels.at(i).bgr[j]);
		}
	}

	for(int i=0; i<3; i++)
	{
		int length = maxi[i] - mini[i];
		if( length > max_num ){
			max_num = length;
			b.longest_dim = i;
		}
	}
	volume = (maxi[0]-mini[0])*(maxi[1]-mini[1])*(maxi[2]-mini[2]);
	if( volume < 0 ){
		cout << "ERROR: " << b.start << " to " << b.end << " volume < 0";
	}

	b.volume = volume;
	b.nPixel = b.end - b.start;
// 1. check with simple box, works fine.
//-----
}

void getNewColor(int* color, const box& b){
	double sumColor[] = {0.0, 0.0, 0.0};
	for(int i=b.start; i<b.end; i++){
		sumColor[0] += (double)pixels.at(i).bgr[0];
		sumColor[1] += (double)pixels.at(i).bgr[1];
		sumColor[2] += (double)pixels.at(i).bgr[2];
	}
	for(int i=0; i<3; i++){
		sumColor[i] /= b.nPixel;
		color[i] = (int)sumColor[i];
	}
	// cout << "color is " << "[" << color[0] << ", " << color[1] << ", " << color[2] << "]" << endl;
// 1. check with simple box, works fine.
//-----
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
// 1. pixels.size() == image.size()
//-----

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
// 2. since we dont use parent.end, (end-start+1)/2 works better than (start+end)/2
// 1. cmp finction and sort work fine
//-----

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
	// then draw all average colors
	int nBox = boxes.size();
	for(int i=0; i<nBox; i++)
	{
		box drawBox = boxes.top();
		boxes.pop();

		int color[3] = {0.0, 0.0, 0.0};
		getNewColor(color, drawBox);

		for(int j=0; j<drawBox.nPixel; j++){
			for(int k=0; k<3; k++){
				pixels.at(drawBox.start+j).bgr[k] = color[k];
			}
		}
	}
	int h = image.size().height, w = image.size().width;
	Mat result_img(h, w, CV_8UC3, Scalar(0, 0, 0));
	for(int row=0; row<h; row++){
		for(int col=0; col<w; col++){
			for(int k=0; k<3; k++){
				result_img.at<Vec3b>(row, col).val[k] = (uchar)pixels.at(row*h+col).bgr[k];
			}
		}
	}
	imwrite("./img/result1.png", result_img);


/* Show result: METHOD 1
	Mat result_img(50*boxes.size(), 300, CV_8UC3, Scalar(0, 0, 0));
	int boxSize = boxes.size();
	for(int bNum=0; bNum<boxSize; bNum++)
	{
		box drawBox = boxes.top();

		int color[3] = {0.0, 0.0, 0.0};
		getNewColor(color, drawBox);

		for(int i=0; i<50; i++){
			for(int j=0; j<300; j++){
				result_img.at<Vec3b>(bNum*50+i, j).val[0] = (uchar)color[0];
				result_img.at<Vec3b>(bNum*50+i, j).val[1] = (uchar)color[1];
				result_img.at<Vec3b>(bNum*50+i, j).val[2] = (uchar)color[2];
			}
		}

		boxes.pop();
	}
	imwrite("./img/result.png", result_img);
	// namedWindow("result", CV_WINDOW_AUTOSIZE);
	// imshow("result", result_img);

	// waitKey(0);
*/

	return 0;
}
