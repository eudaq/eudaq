#include "Planes.h"
#include "TH2D.h"
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "XMLextractorHelpers.h"
using namespace std;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}

void plane::addCurrentEntry(const int& plane_id, const int& hit_x, const int& hit_y)
{
  if (m_plane_id == plane_id
    &&
    !ignor(hit(hit_x, hit_y))
    )
  {
    pos.push_back(hit(hit_x, hit_y));
    Hitmap->Fill(hit_x, hit_y);
  }
}

void plane::addCurrentEntry2CalibrationData(const int& plane_id, const int& hit_x, const int& hit_y)
{
  if (m_plane_id == plane_id
    &&
    !ignor(hit(hit_x, hit_y)))
  {
    ev_.emplace_back(hit_x, hit_y);

  }
}

plane::plane(int plane_id) :m_plane_id(plane_id), m_Ignore_percentage(1)
{
  //	h1_x=new TH1D("h1_x","hotPixel",2000,0,1999);
  //	h1_y=new TH1D("h1_y","hotPixel",2000,0,1999);

}
plane::plane(rapidxml::xml_node<> *node) :m_Ignore_percentage(1){

  m_plane_id = std::stoi(node->first_attribute("id")->value());
  //////////////////////////////////////////////////////////////////////////
  // binning
  auto binning = node->first_node("Binning");
  m_x_axis.low = XMLhelper::getValue(binning->first_node("PixelX")->first_attribute("LowerBin"), (double)0);
  m_x_axis.high = XMLhelper::getValue(binning->first_node("PixelX")->first_attribute("upperBin"), (double)1000);
  m_x_axis.axis_title = XMLhelper::getValue(binning->first_node("PixelX")->first_attribute("title"), string("x"));

  double pixelSize = XMLhelper::getValue(binning->first_node("PixelX")->first_attribute("binSize"), (double)1);

  m_x_axis.bins = (m_x_axis.high - m_x_axis.low + 1) / pixelSize;


  m_y_axis.low = stod(binning->first_node("PixelY")->first_attribute("LowerBin")->value());
  m_y_axis.high = stod(binning->first_node("PixelY")->first_attribute("upperBin")->value());

  pixelSize = stod(binning->first_node("PixelY")->first_attribute("binSize")->value());

  m_y_axis.bins = (m_y_axis.high - m_y_axis.low + 1) / pixelSize;
  // binning end
  //////////////////////////////////////////////////////////////////////////


  //////////////////////////////////////////////////////////////////////////
  // Ignore Start

  auto ignor = node->first_node("ignorHotPixel");
  if (ignor)
  {
    ignor = ignor->first_node("ignorReginX");
    while (ignor)
    {
      double begin_ignor = stod(ignor->first_attribute("Begin")->value());
      double end_ignor = stod(ignor->first_attribute("End")->value());

      add2X_IgnorRegin(begin_ignor, end_ignor);
      ignor = ignor->next_sibling("ignorReginX");
    }
    ignor = node->first_node("ignorHotPixel")->first_node("ignorReginY");

    while (ignor)
    {
      double begin_ignor = stod(ignor->first_attribute("Begin")->value());
      double end_ignor = stod(ignor->first_attribute("End")->value());

      add2Y_IgnorRegin(begin_ignor, end_ignor);
      ignor = ignor->next_sibling("ignorReginY");
    }

    ignor = node->first_node("ignorHotPixel")->first_node("ignorPixel");

    while (ignor)
    {
      auto s = string(ignor->value());
      s.erase(s.find_last_not_of(" ;\n\r\t") + 1);
      std::vector<std::string> x = split(s, ';');
      for (auto& pix : x){
        ignor_.push_back(createHitFromString(pix));
      }
      ignor = ignor->next_sibling("ignorPixel");
    }

    ignor = node->first_node("ignorHotPixel")->first_node("ignorePercentage");
    if (ignor)
    {
      m_Ignore_percentage = std::stod(ignor->first_attribute("percentage")->value()) / 100;
    }
  }
  // ignore end
  //////////////////////////////////////////////////////////////////////////


}
bool plane::nextEntry()
{
  return (++current_pos != pos.end());
}

void plane::firstEntry()
{
  current_pos = pos.begin();
}

double plane::getX() const
{
  return current_pos->x;
}

double plane::getY() const
{
  return current_pos->y;
}

bool plane::empty()
{
  return pos.empty();
}

void plane::clear()
{
  pos.clear();
}

plane::~plane()
{


}

void plane::HotPixelsuppression()
{


  std::vector<int> entries;
  std::vector<hit> dummy;
  int sizeOfInput = ev_.size();
  while (!ev_.empty())
  {
    hit current = ev_.back();
    if (std::find(dummy.begin(), dummy.end(), current) == dummy.end()){
      entries.push_back(std::count(ev_.begin(), ev_.end(), current));
      dummy.push_back(current);

    }
    ev_.pop_back();


  }
  std::cout << " sizeOfInput: " << sizeOfInput << std::endl;
  for (size_t i = 0; i < entries.size(); i++)
  {
    if (entries.at(i) > sizeOfInput*m_Ignore_percentage)
    {
      ignor_.push_back(dummy.at(i));
    }
  }

}

void plane::extraxtHotPixel(std::vector<hit>& h1, std::vector<hit>& ignor)
{



}

bool plane::ignor(hit x)
{
  if (ignor_.empty()
    &&
    ignorX_begin.empty()
    &&
    ignorY_begin.empty())
  {
    return false;
  }


  if (find(ignor_.begin(), ignor_.end(), x) != ignor_.end()){
    return true;
  }
  if (ignorRegin(x.x, ignorX_begin, ignorX_end))
  {
    return true;
  }

  if (ignorRegin(x.y, ignorY_begin, ignorY_end))
  {
    return true;
  }

  return  false;
}


void plane::setHistogramSize(double XLowerBin, double XUpperBin, int XnumberOfBins, double YLowerBin, double YUpperBin, int YnumberOfBins)
{
  m_x_axis.low = XLowerBin;

  m_x_axis.high = XUpperBin;
  m_x_axis.bins = XnumberOfBins;
  m_y_axis.low = YLowerBin;
  m_y_axis.high = YUpperBin;
  m_y_axis.bins = YnumberOfBins;

}

void plane::add2X_IgnorRegin(double beginIgnor, double endIgnor)
{
  ignorX_begin.push_back(beginIgnor);
  ignorX_end.push_back(endIgnor);
}

void plane::add2Y_IgnorRegin(double beginIgnor, double endIgnor)
{
  ignorY_begin.push_back(beginIgnor);
  ignorY_end.push_back(endIgnor);
}

void plane::setIgnorePercentage(int Percentage)
{
  m_Ignore_percentage = Percentage;
}

void plane::createHistograms()
{
  std::string name = "hitmap_" + to_string(m_plane_id);
  std::string title = "hit map " + to_string(m_plane_id);
  Hitmap = new TH2D(name.c_str(), title.c_str(), m_x_axis.bins, m_x_axis.low, m_x_axis.high, m_y_axis.bins, m_y_axis.low, m_y_axis.high);
}

void plane::Draw(const char* DrawOptions/*=""*/)
{
  Hitmap->Draw(DrawOptions);
}


hit createHitFromString(const std::string& commaSeparatedString)
{
  auto vec = split(commaSeparatedString, ',');
  if (vec.size() != 2)
  {
    throw("wrong input for hit");
  }
  hit ret(std::stod(vec.at(0)), std::stod(vec.at(1)));

  return ret;

}
