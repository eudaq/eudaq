#include <math.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <stdarg.h>
#include <fstream>
#include <sstream>

using namespace std;

struct pixel {
	double x;
	double y;
};

const double PI = 4*atan(1);
const double cospi3 = cos(PI/3);


int fill_pixels(int num_of_pixels, int row, int pixel_num, double y_max, double density, pixel *pixels)
{
	for (int i = 0; i < num_of_pixels; i++)
	{
		double stepsize = i*density;
		
		pixels[pixel_num].x = (row*cospi3);
		pixels[pixel_num].y = (y_max-stepsize);
		
		//cout << " PIXEL_COUNTER: " << pixel_num << " X position " << row*cospi3 << endl;
		//cout << " PIXEL_COUNTER: " << pixel_num << " X position " << pixels[pixel_num].x << endl;
		pixel_num++;
		
	}
	return pixel_num;
}

/* void map_sensor_to_kpix(pixel *pixel_sensor, pixel *pixel_kpix) // sensor_to_kpix (pixel *pixel_sensor) */
/* { */
/* 	string line; */
	
/* 	int line_count = 0; */
/* 	//cout << "DEBUG3: " << pixel_sensor[line_count].x << endl; */
/* 	ifstream infile("/home/kraemeru/afs/bin/gui_hama.txt"); */
/* 	if (infile.good()) */
/* 	{ */
/* 		while(getline(infile,line)) */
/* 		{ */
/* 			int col0; */
/* 			int col1; */
/* 			istringstream linestream(line); */
/* 			linestream >> col0; */
/* 			linestream >> col1; */
/* 			////cout << pixel_sensor[line_count].x << endl; */
/* 			////cout << col1 << endl; */
/* 			////cout << line_count << endl; */
			
/* 			pixel_kpix[col1].x = pixel_sensor[line_count].x; */
/* 			pixel_kpix[col1].y = pixel_sensor[line_count].y; */
/* 			//cout << "DEBUG2: " << pixel_sensor[line_count].x << endl; */
/* 			line_count++; */
/* 		} */
		
/* 	} */
/* 	else  */
/* 		{ */
/* 			//cout << "MISSING MAPPING FILE!" << endl; */
/* 		} */
		
	
/* } */

void map_kpix_to_strip(int *strip) // sensor_to_kpix (pixel *pixel_sensor)
{
	string line;
	//cout << "DEBUG3: " << pixel_sensor[line_count].x << endl;
	//ifstream infile("/home/kraemeru/KPiX-Analysis/include/tracker_to_kpix_left.txt");
	ifstream infile("./data/tracker_to_kpix_left.txt");
	if (infile.good())
	{
		while(getline(infile,line))
		{
			istringstream linestream(line);
			int col[2];
			int count = 0;
			while (linestream >> col[count] || !linestream.eof())
			{
				if (linestream.fail())
				{
					linestream.clear();
					string dummy;
					linestream >> dummy;
					continue;
				}
				count++;
			}
			strip[col[0]] = col[1];
		}
	}
	else
	{
			cout << "MISSING MAPPING FILE!" << endl;
	}
}


/* // //int main() */
/* void pixel_mapping(pixel *pixel_ext)  */
/* { */
/* 	////cout << "test2" << endl; */
/* 	double y_max = 0; */
/* 	double y_min = -6; */
/* 	double num_of_pixels = y_max - y_min; */
/* 	pixel pixel_sensor[1024]; */
/* 	//pixel *pixel_sensor; */
/* 	//pixel *pixel_kpix; */
/* 	////cout << "test3" << endl; */
/* 	int pixel_counter = 0; */
/* 	for (int count = 0; count<39; count++) */
/* 	{ */
/* 		if ( count == 0 ) */
/* 		{ */
/* 			pixel_counter = fill_pixels(num_of_pixels, count, pixel_counter, y_max, 1, pixel_sensor); */
/* 			////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 		} */
/* 		else if ((count > 0) && (count <= 7)) */
/* 		{ */
/* 			y_max = y_max + 1.5; */
/* 			y_min = y_min - 1.5; */
/* 			num_of_pixels = y_max - y_min; */
			
/* 			pixel_counter = fill_pixels(num_of_pixels, count, pixel_counter, y_max, 1, pixel_sensor); */
			
/* 		} */
/* 		else if ((count >= 8) && (count <= 11)) */
/* 		{ */
/* 			y_max = y_max + 0.5; */
/* 			y_min = y_min - 0.5; */
/* 			num_of_pixels = y_max - y_min; */
/* 			pixel_counter = fill_pixels(num_of_pixels, count, pixel_counter, y_max, 1, pixel_sensor); */
/* 			////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 		} */
/* 		else if ((count >= 12) && (count < 28)) */
/* 		{ */
/* 			if (count % 2 == 1) */
/* 			{ */
/* 				y_max = y_max - 0.5; */
/* 				y_min = y_min + 0.5; */
/* 			} */
/* 			else */
/* 			{ */
/* 				y_max = y_max + 0.5; */
/* 				y_min = y_min - 0.5; */
/* 			} */
/* 			if ((count == 15) || (count == 23)) */
/* 			{ */
/* 				num_of_pixels = y_max - y_min; */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+0.5), count, pixel_counter, y_max, 1, pixel_sensor); */
/* 				pixel_counter = fill_pixels((y_max-num_of_pixels/2+1)-(y_min+num_of_pixels/2-1), count, pixel_counter, (y_max-num_of_pixels/2+0.5), 0.5, pixel_sensor); */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+0.5), count, pixel_counter, y_min+num_of_pixels/2-0.5, 1, pixel_sensor); */
/* 				////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 			} */
/* 			else if ((count == 16) || (count == 22)) */
/* 			{ */
/* 				num_of_pixels = y_max - y_min; */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+3), count, pixel_counter, y_max, 1, pixel_sensor); */
/* 				pixel_counter = fill_pixels((y_max-num_of_pixels/2+6)-(y_min+num_of_pixels/2-6), count, pixel_counter, (y_max-num_of_pixels/2+3), 0.5, pixel_sensor); */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+3), count, pixel_counter, y_min+num_of_pixels/2-3, 1, pixel_sensor); */
/* 				////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 			} */
/* 			else if ((count == 17) || (count == 19) || (count == 21)) */
/* 			{ */
/* 				num_of_pixels = y_max - y_min; */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+3.5), count, pixel_counter, y_max, 1, pixel_sensor); */
/* 				pixel_counter = fill_pixels((y_max-num_of_pixels/2+7)-(y_min+num_of_pixels/2-7), count, pixel_counter, (y_max-num_of_pixels/2+3.5), 0.5, pixel_sensor); */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+3.5), count, pixel_counter, y_min+num_of_pixels/2-3.5, 1, pixel_sensor); */
/* 				////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 			} */
/* 			else if ((count == 18) || (count == 20)) */
/* 			{ */
/* 				num_of_pixels = y_max - y_min; */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+4), count, pixel_counter, y_max, 1, pixel_sensor); */
/* 				pixel_counter = fill_pixels((y_max-num_of_pixels/2+8)-(y_min+num_of_pixels/2-8), count, pixel_counter, (y_max-num_of_pixels/2+3.5), 0.5, pixel_sensor); */
/* 				pixel_counter = fill_pixels(y_max-(y_min+num_of_pixels/2+4), count, pixel_counter, y_min+num_of_pixels/2-4, 1, pixel_sensor); */
/* 				////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 			} */
/* 			else */
/* 			{ */
/* 				num_of_pixels = y_max - y_min; */
/* 				pixel_counter = fill_pixels(num_of_pixels, count, pixel_counter, y_max, 1, pixel_sensor); */
/* 				////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 			} */
/* 		} */
/* 		else if ((count >= 28) && (count <= 31)) */
/* 		{ */
/* 			y_max = y_max - 0.5; */
/* 			y_min = y_min + 0.5; */
/* 			num_of_pixels = y_max - y_min; */
/* 			pixel_counter = fill_pixels(num_of_pixels, count, pixel_counter, y_max, 1, pixel_sensor); */
/* 			////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 		} */
/* 		else if (count > 31) */
/* 		{ */
/* 			y_max = y_max - 1.5; */
/* 			y_min = y_min + 1.5; */
/* 			num_of_pixels = y_max - y_min; */
/* 			pixel_counter = fill_pixels(num_of_pixels, count, pixel_counter, y_max, 1, pixel_sensor); */
/* 			////cout << "ROW: " << count << " PIXEL_COUNTER: " << pixel_counter << endl; */
/* 		} */
/* 		//cout << "DEBUG: " << pixel_sensor[pixel_counter-1].x << endl; */
		
/* 	} */
	
/* 	map_sensor_to_kpix(pixel_sensor, pixel_ext); */
/* 	//cout << "TEST: " << pixel_sensor[pixel_counter-1].x << endl; */
/* } */


