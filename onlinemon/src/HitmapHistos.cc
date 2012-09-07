/*
 * HitmapHistos.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include "HitmapHistos.hh"
#include "OnlineMon.hh"
#include <cstdlib>

HitmapHistos::HitmapHistos(SimpleStandardPlane p, RootMonitor* mon): _sensor(p.getName()), _id(p.getID()), _maxX(p.getMaxX()), _maxY(p.getMaxY()), _wait(false),
_hitmap(NULL),_clusterMap(NULL),_lvl1Distr(NULL), _lvl1Width(NULL),_lvl1Cluster(NULL),_totSingle(NULL),_totCluster(NULL),
_hitOcc(NULL), _nClusters(NULL), _nHits(NULL), _clusterXWidth(NULL), _clusterYWidth(NULL),_nbadHits(NULL),_nHotPixels(NULL),_hitmapSections(NULL),
is_MIMOSA26(false), is_APIX(false), is_USBPIX(false),is_USBPIXI4(false)
{
	char out[1024], out2[1024];


	_mon=mon;
	mimosa26_max_section=_mon->mon_configdata.getMimosa26_max_sections();
	if (_sensor == std::string("MIMOSA26"))
	{
		is_MIMOSA26=true;
	}
	else if (_sensor == std::string("APIX"))
	{
		is_APIX=true;
	}
	else if (_sensor == std::string("USBPIX"))
	{
		is_USBPIX=true;
	}
	else if (_sensor == std::string("USBPIXI4"))
	{
		is_USBPIXI4=true;
	}
	is_DEPFET = p.is_DEPFET;

	std::cout << "HitmapHistos::Sensorname: " << _sensor << " "<< _id<< std::endl;

	if (_maxX != -1 && _maxY != -1) {
		sprintf(out,"%s %i Raw Hitmap",_sensor.c_str(),_id);
		sprintf(out2,"h_hitmap_%s_%i",_sensor.c_str(), _id);
		_hitmap = new TH2I(out2, out, _maxX+1,0,_maxX, _maxY+1,0,_maxY);
		SetHistoAxisLabels(_hitmap,"X","Y");
		//std::cout << "Created Histogram " << out2 << std::endl;

		sprintf(out,"%s %i Cluster Hitmap",_sensor.c_str(),_id);
		sprintf(out2,"h_clustermap_%s_%i",_sensor.c_str(), _id);
		_clusterMap = new TH2I(out2, out, _maxX+1,0,_maxX, _maxY+1,0,_maxY);
		SetHistoAxisLabels(_clusterMap,"X","Y");
		//std::cout << "Created Histogram " << out2 << std::endl;

		sprintf(out,"%s %i hot Pixel Map",_sensor.c_str(),_id);
		sprintf(out2,"h_hotpixelmap_%s_%i",_sensor.c_str(), _id);
		_HotPixelMap = new TH2D(out2, out, _maxX+1,0,_maxX, _maxY+1,0,_maxY);
		SetHistoAxisLabels(_HotPixelMap,"X","Y");



		sprintf(out,"%s %i LVL1 Pixel Distribution",_sensor.c_str(), _id);
		sprintf(out2,"h_lvl1_%s_%i",_sensor.c_str(), _id);
		_lvl1Distr = new TH1I(out2, out,16,0,16);

		sprintf(out,"%s %i LVL1 Cluster Distribution",_sensor.c_str(), _id);
		sprintf(out2,"h_lvl1cluster_%s_%i",_sensor.c_str(), _id);
		_lvl1Cluster = new TH1I(out2, out,16,0,16);

		sprintf(out,"%s %i LVL1 Clusterwidth",_sensor.c_str(), _id);
		sprintf(out2,"h_lvl1width_%s_%i",_sensor.c_str(), _id);
		_lvl1Width = new TH1I(out2, out,16,0,16);

		sprintf(out,"%s %i TOT Single Pixels",_sensor.c_str(), _id);
		sprintf(out2,"h_totsingle_%s_%i",_sensor.c_str(), _id);
		if (p.is_USBPIXI4)
		{
			_totSingle = new TH1I(out2, out,16,0,15);
		}
		else if (p.is_DEPFET)
		{
			_totSingle = new TH1I(out2, out,255,-127,127);
		}
		else
		{
			_totSingle = new TH1I(out2, out,256,0,255);
			_totSingle->SetBit(TH1::kCanRebin);
		}


		sprintf(out,"%s %i TOT Clusters",_sensor.c_str(), _id);
		sprintf(out2,"h_totcluster_%s_%i",_sensor.c_str(), _id);
		if (p.is_USBPIXI4)
			_totCluster= new TH1I(out2, out,80,0,79);
		else
			_totCluster= new TH1I(out2, out,256,0,255);

		sprintf(out,"%s %i Hitoccupancy",_sensor.c_str(), _id);
		sprintf(out2,"h_hitocc%s_%i",_sensor.c_str(), _id);

		_hitOcc= new TH1F(out2, out,250,0.01, 1);
        SetHistoAxisLabelx(_hitOcc,"Frequency");

		sprintf(out,"%s %i Clustersize",_sensor.c_str(), _id);
		sprintf(out2,"h_clustersize_%s_%i",_sensor.c_str(), _id);
		_clusterSize= new TH1I(out2, out,10,0,10);
		SetHistoAxisLabelx(_clusterSize,"Cluster Size");

		sprintf(out,"%s %i Number of Hits",_sensor.c_str(), _id);
		sprintf(out2,"h_nHits_%s_%i",_sensor.c_str(), _id);
		_nHits = new TH1I(out2, out,500,0,99);
		SetHistoAxisLabelx(_nHits,"n_Hits");

		sprintf(out,"%s %i Number of Invalid Hits",_sensor.c_str(), _id);
		sprintf(out2,"h_nbadHits_%s_%i",_sensor.c_str(), _id);
		_nbadHits = new TH1I(out2, out,50,0,50);
		SetHistoAxisLabelx(_nbadHits,"n_{BadHits}");

		sprintf(out,"%s %i Number of Hot Pixels",_sensor.c_str(), _id);
		sprintf(out2,"h_nhotpixels_%s_%i",_sensor.c_str(), _id);
		_nHotPixels = new TH1I(out2, out,50,0,50);
		SetHistoAxisLabelx(_nHotPixels,"n_{HotPixels}");

		sprintf(out,"%s %i Number of Clusters",_sensor.c_str(), _id);
		sprintf(out2,"h_nClusters_%s_%i",_sensor.c_str(), _id);
		_nClusters = new TH1I(out2, out,500,0,99);
		SetHistoAxisLabelx(_nClusters,"n_{Clusters}");

		sprintf(out,"%s %i Clustersize in X",_sensor.c_str(), _id);
		sprintf(out2,"h_clustersizeX_%s_%i",_sensor.c_str(), _id);
		_clusterXWidth = new TH1I(out2, out,20,0,19);

		sprintf(out,"%s %i Clustersize in Y",_sensor.c_str(), _id);
		sprintf(out2,"h_clustersizeY_%s_%i",_sensor.c_str(), _id);
		_clusterYWidth = new TH1I(out2, out,20,0,19);

		sprintf(out,"%s %i Hitmap Sections",_sensor.c_str(), _id);
		sprintf(out2,"h_hitmapSections_%s_%i",_sensor.c_str(), _id);
		_hitmapSections = new TH1I(out2, out,mimosa26_max_section,_id,_id+1);

		sprintf(out,"%s %i Pivot Pixel Usage",_sensor.c_str(), _id);
		sprintf(out2,"h_pivotpixel_%s_%i",_sensor.c_str(), _id);
		_nPivotPixel= new TH1I(out2, out,930,0,9300);


		for (unsigned int section=0; section<mimosa26_max_section;section++)
		  {
		    sprintf(out,"%i%c",_id,(section+65));
		    _hitmapSections->GetXaxis()->SetBinLabel(section+1,out);
		  }
	
		_nHits_section =new TH1I*[mimosa26_max_section];
		_nClusters_section=new TH1I*[mimosa26_max_section];
		_nClustersize_section=new TH1I*[mimosa26_max_section];
		_nHotPixels_section=new TH1I*[mimosa26_max_section];

		for (unsigned int section=0; section<mimosa26_max_section; section++)
		{
			sprintf(out,"%s %i Number of Hits in Section %i ",_sensor.c_str(), _id,section);
			sprintf(out2,"h_hits_section%i_%s_%i",section,_sensor.c_str(), _id);
			_nHits_section[section]= new TH1I(out2, out,50,0,50);
			if (_nHits_section[section]==NULL)
			{
				cout<< "Error allocating Histogram" << out<< endl;
				exit(-1);
			}
			else
			{
				_nHits_section[section]->GetXaxis()->SetTitle("Hits");
			}

			sprintf(out,"%s %i Number of Clusters in Section %i ",_sensor.c_str(), _id,section);
			sprintf(out2,"h_clusters_section%i_%s_%i",section,_sensor.c_str(), _id);
			_nClusters_section[section]= new TH1I(out2, out,50,0,50);
			if (_nClusters_section[section]==NULL)
			{
				cout<< "Error allocating Histogram" << out<< endl;
				exit(-1);
			}
			else
			{
				_nClusters_section[section]->GetXaxis()->SetTitle("Clusters");
			}

			sprintf(out,"%s %i Cluster size in Section %i ",_sensor.c_str(), _id,section);
			sprintf(out2,"h_clustersize_section%i_%s_%i",section,_sensor.c_str(), _id);
			_nClustersize_section[section]= new TH1I(out2, out,10,0,10);
			if (_nClustersize_section[section]==NULL)
			{
					cout<< "Error allocating Histogram" << out<< endl;
					exit(-1);
			}
			else
			{
				_nClustersize_section[section]->GetXaxis()->SetTitle("Cluster Size");
			}

			sprintf(out,"%s %i Number of Hot Pixels in Section %i ",_sensor.c_str(), _id,section);
			sprintf(out2,"h_hotpixels_section%i_%s_%i",section,_sensor.c_str(), _id);
			_nHotPixels_section[section]= new TH1I(out2, out,50,0,50);
			if (_nHotPixels_section[section]==NULL)
			{
				cout<< "Error allocating Histogram" << out<< endl;
				exit(-1);
			}
			else
			{
				_nHotPixels_section[section]->GetXaxis()->SetTitle("Hot Pixels");
			}

		}
// make a plane array for calculating e..g hotpixels and occupancy

		plane_map_array=new int*[_maxX];
		if (plane_map_array!=NULL)
		{
			for (int j=0; j< _maxX; j++)
			{
				plane_map_array[j]=new int[_maxY];
				if (plane_map_array[j]==NULL)
				{
					cout << "HitmapHistos :Error in memory allocation"<<endl;
					exit(-1);
				}
			}
			zero_plane_array();
		}






	} else {
		std::cout << "No max sensorsize known!" << std::endl;
	}

}

int HitmapHistos::zero_plane_array()
{
	for (int i=0; i<_maxX; i++)
	{
		for (int j=0; j< _maxY; j++)
		{
			plane_map_array[i][j]=0;
		}
	}
	return 0;
}

void HitmapHistos::Fill(const SimpleStandardHit & hit)
{
    int pixel_x=hit.getX();
    int pixel_y=hit.getY();
	
	bool pixelIsHot = false;
	if (_HotPixelMap->GetBinContent(pixel_x+1,pixel_y+1)>_mon->mon_configdata.getHotpixelcut()) pixelIsHot=true;

    if (_hitmap != NULL && !pixelIsHot) _hitmap->Fill(pixel_x,pixel_y);
    if ((is_MIMOSA26) && (_hitmapSections != NULL) && (!pixelIsHot))
        //&& _hitOcc->GetEntries()>0) // only fill histogram when occupancies and hotpixels have been determined
      {
        char sectionid[3];
        sprintf(sectionid,"%i%c",_id,(pixel_x/_mon->mon_configdata.getMimosa26_section_boundary())+65); // determine section label
	    _hitmapSections->Fill(sectionid,1); // add one hit to the corresponding section bin
        //tab[pixel_x/_mon->mon_configdata.getMimosa26_section_boundary()]++;
    }
    /*else
    {
        bool MIMOSA, PIXEL, HITMAP;
        MIMOSA = true;
        PIXEL = true;
        HITMAP = true;

        if (!is_MIMOSA26)
            cout << "MIMOSA IS FALSE" << endl;
        if (_hitmapSections == NULL)
            cout << "_hitmapSections IS NULL" << endl;
        if (pixelIsHot)
            cout << "pixelIsHot IS TRUE" << endl;

    }*/

	if ((pixel_x<_maxX) &&  (pixel_y<_maxY))
	{
		plane_map_array[pixel_x][pixel_y]=plane_map_array[pixel_x][pixel_y]+1;
	}
	//if (_sensor == std::string("APIX")) {
	//if (_sensor == std::string("APIX") || _sensor == std::string("USBPIX") || _sensor == std::string("USBPIXI4") ) {
	if ( (is_APIX) || (is_USBPIX) || (is_USBPIXI4) || is_DEPFET)
	{
		if (_totSingle != NULL) _totSingle->Fill(hit.getTOT());
		if (_lvl1Distr != NULL) _lvl1Distr->Fill(hit.getLVL1());
	}
}

void HitmapHistos::Fill(const SimpleStandardPlane & plane)
{
    //std::cout<< "FILL with a plane." << std::endl;
	if (_nHits != NULL) _nHits->Fill(plane.getNHits());
	if ((_nbadHits != NULL) &&(plane.getNBadHits()>0))
	{
		_nbadHits->Fill(plane.getNBadHits());
	}
	if (_nClusters != NULL) _nClusters->Fill(plane.getNClusters());

	// we fill the information for the individual mimosa sections, and do a zero-suppression,in case not all sections have hits/clusters
	if (is_MIMOSA26)
	{
		_nPivotPixel->Fill(plane.getPivotPixel());
		for (unsigned int section=0; section<mimosa26_max_section; section++)
		{
			if (_nHits_section[section]!=NULL)
			{
				if (plane.getNSectionHits(section)> 0)
				{
                    _nHits_section[section]->Fill(plane.getNSectionHits(section));
                    //std::cout<< "Section " << section << " filling with " << plane.getNSectionHits(section) << std::endl;
				}
			}
			if (_nClusters_section[section]!=NULL)
			{
				if (plane.getNSectionClusters(section)>0)
				{
						_nClusters_section[section]->Fill(plane.getNSectionClusters(section));
				}
			}
		}
	}

}

void HitmapHistos::Fill(const SimpleStandardCluster & cluster)
{
	if (_clusterMap != NULL) _clusterMap->Fill(cluster.getX(), cluster.getY());
	if (_clusterSize != NULL) _clusterSize->Fill(cluster.getNPixel());
	if (is_MIMOSA26)
	{
		unsigned int nsection=cluster.getX()/_mon->mon_configdata.getMimosa26_section_boundary(); //get to which section in Mimosa the cluster belongs
		if ((nsection <mimosa26_max_section) && (_nClustersize_section[nsection]!=NULL)) //check if valid address
		{
			if (cluster.getNPixel()>0)
			{
				_nClustersize_section[nsection]->Fill(cluster.getNPixel());
			}
		}
	}


	if ( (is_APIX) || is_USBPIX || is_USBPIXI4 )
	{
		if (_lvl1Width != NULL)   _lvl1Width->Fill(cluster.getLVL1Width());
		if (_totCluster != NULL ) _totCluster->Fill(cluster.getTOT());
		if (_lvl1Cluster != NULL) _lvl1Cluster->Fill(cluster.getFirstLVL1());
		if (_clusterXWidth != NULL) _clusterXWidth->Fill(cluster.getWidthX());
		if (_clusterYWidth != NULL) _clusterYWidth->Fill(cluster.getWidthY());
	}
}


void HitmapHistos::Reset() {
	_hitmap->Reset();
	_totSingle->Reset();
	_lvl1Distr->Reset();
	_clusterMap->Reset();
	_totCluster->Reset();
	_lvl1Cluster->Reset();
	_lvl1Width->Reset();
	_hitOcc->Reset();
	_clusterSize->Reset();
	_nClusters->Reset();
	_nHits->Reset();
	_nbadHits->Reset();
	_nHotPixels->Reset();
	_HotPixelMap->Reset();
	_clusterYWidth->Reset();
	_clusterXWidth->Reset();
	_hitmapSections->Reset();
	_nPivotPixel->Reset();
	for (unsigned int  section=0; section<mimosa26_max_section; section++)
	{
		_nClusters_section[section]->Reset();
		_nHits_section[section]->Reset();
		_nClustersize_section[section]->Reset();
		_nHotPixels_section[section]->Reset();
	}
	// we have to reset the aux array as well
	zero_plane_array();
}





void HitmapHistos::Calculate(const int currentEventNum)
{
	_wait = true;
	_hitOcc->SetBins(currentEventNum/10,0,1);
	_hitOcc->Reset();


	int nHotpixels=0;
	std::vector <unsigned int> nHotpixels_section;
	double Hotpixelcut= _mon->mon_configdata.getHotpixelcut();
	double bin=0;
	double occupancy=0;
	if (is_MIMOSA26) //probalbly initialize vector
	{
		nHotpixels_section.reserve(_mon->mon_configdata.getMimosa26_max_sections());
		for (unsigned int i=0; i< _mon->mon_configdata.getMimosa26_max_sections(); i++)
		{
			nHotpixels_section[i]=0;
		}
	}

	for (int x = 0; x < _maxX; ++x) {
		for (int y = 0; y < _maxY; ++y) {

			bin=plane_map_array[x][y];

			if (bin != 0)
			{
                occupancy=bin/(double)currentEventNum; //FIXME it's not occupancy, it's frequency
				_hitOcc->Fill(occupancy);
				// only count as hotpixel if occupancy larger than minimal occupancy for a single hit
                if (occupancy>Hotpixelcut && ((1./(double)(currentEventNum))<_mon->mon_configdata.getHotpixelcut()))
                //if (occupancy>Hotpixelcut && )
				{
					nHotpixels++;
					_HotPixelMap->SetBinContent(x+1,y+1,occupancy); // ROOT start from 1
					if (is_MIMOSA26)
					{
						nHotpixels_section[x/_mon->mon_configdata.getMimosa26_section_boundary()]++;
					}
				}
			}
		}
	}
	if (nHotpixels>0)
	{
		_nHotPixels->Fill(nHotpixels);
		if (is_MIMOSA26)
		{
			for (unsigned int section=0; section< _mon->mon_configdata.getMimosa26_max_sections(); section++)
				{
					if ((nHotpixels_section[section]>0))
					{
						_nHotPixels_section[section]->Fill(nHotpixels_section[section]);
                        //cout<<"nHotPixels is being filled for plane " << _id << " with " <<  ++counter_nhotpixels_being_filled << " time in section " <<
                        //      section << endl;
					}
				}
		}
	}

	_wait = false;


}


void HitmapHistos::Write()
{
	_hitmap->Write();
	_totSingle->Write();
	_lvl1Distr->Write();
	_clusterMap->Write();
	_totCluster->Write();
	_lvl1Cluster->Write();
	_lvl1Width->Write();
	_hitOcc->Write();
	_clusterSize->Write();
	_nClusters->Write();
	_nHits->Write();
	_nbadHits->Write();
	_HotPixelMap->Write();
	_nHotPixels->Write();
	_clusterXWidth->Write();
	_clusterYWidth->Write();
	_hitmapSections->Write();
	_nPivotPixel->Write();
	for (unsigned int  section=0; section<mimosa26_max_section; section++)
	{
		_nClusters_section[section]->Write();
		_nHits_section[section]->Write();
		_nClustersize_section[section]->Write();
		_nHotPixels_section[section]->Write();
	}
}

int HitmapHistos::SetHistoAxisLabelx(TH1* histo,string xlabel)
{
	if (histo!=NULL)
	{
		histo->GetXaxis()->SetTitle(xlabel.c_str());
	}
	return 0;
}



int HitmapHistos::SetHistoAxisLabels(TH1* histo,string xlabel, string ylabel)
{
	SetHistoAxisLabelx(histo,xlabel);
	SetHistoAxisLabely(histo,ylabel);

	return 0;
}



int HitmapHistos::SetHistoAxisLabely(TH1* histo,string ylabel)
{
	if (histo!=NULL)
	{
		histo->GetYaxis()->SetTitle(ylabel.c_str());
	}
	return 0;
}


