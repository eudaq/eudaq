/*
 * HitmapCollection.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include "HitmapCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool HitmapCollection::isPlaneRegistered(SimpleStandardPlane p)
{
	std::map<SimpleStandardPlane,HitmapHistos*>::iterator it;
	it = _map.find(p);
	return (it != _map.end());
}

void HitmapCollection::fillHistograms(const SimpleStandardPlane &simpPlane)
{
    /*
    section_counter[0] = 0;
    section_counter[1] = 0;
    section_counter[2] = 0;
    section_counter[3] = 0;
    */

	if (!isPlaneRegistered(simpPlane))
	{

		registerPlane(simpPlane);

		isOnePlaneRegistered = true;
	}

	HitmapHistos *hitmap = _map[simpPlane];
	hitmap->Fill(simpPlane);

    ++counting;
    events += simpPlane.getNHits();

//    if(counting == 60000)
//        std::cout << "Final AVG: " << std::scientific << (double)events / (double)10000 << std::endl;

	for (int hitpix = 0; hitpix < simpPlane.getNHits();hitpix++)
	{
		const SimpleStandardHit& onehit = simpPlane.getHit(hitpix);

        hitmap->Fill(onehit);
	}

    /*bool flag = true;
    std::cout<< "FILL with plane" <<std::endl;
    for(int i=0; i<4; i++)
    {
        std::cout<< "Section " << i << " filling with " << simpPlane.getNSectionHits(i) << std::endl;

    }

    cout<<"FILL with events." << endl;
    for(int i=0; i<4; i++)
    {
        std::cout<<"Section " << i << " filling with " << section_counter[i] << std::endl;
         simpPlane.getNSectionHits(i) != section_counter[i] )
            flag = false;
        if (flag == true)
            ;//cout << "(DEBUG)Flag: True" << endl;
        else
            ;
            //cout << "(DEBUG)Flag: False" << endl;
    }*/

	for (int cluster = 0; cluster < simpPlane.getNClusters();cluster++)
	{
		const SimpleStandardCluster& onecluster = simpPlane.getCluster(cluster);

		hitmap->Fill(onecluster);
	}

}




void HitmapCollection::bookHistograms(const SimpleStandardEvent &simpev)
{
	for (int plane = 0; plane < simpev.getNPlanes(); plane++)
	{
		SimpleStandardPlane simpPlane = simpev.getPlane(plane);
		if (!isPlaneRegistered(simpPlane))
		{
			registerPlane(simpPlane);

		}

	}
}


void HitmapCollection::Write(TFile *file)
{
	if (file==NULL)
	{
		cout << "HitmapCollection::Write File pointer is NULL"<<endl;
		exit(-1);
	}
	if (gDirectory!=NULL) //check if this pointer exists
	{
		gDirectory->mkdir("Hitmaps");
		gDirectory->cd("Hitmaps");


		std::map<SimpleStandardPlane,HitmapHistos*>::iterator it;
		for (it = _map.begin(); it != _map.end(); ++it) {

			char sensorfolder[255] = "";
			sprintf(sensorfolder,"%s_%d",it->first.getName().c_str(), it->first.getID());
			cout << "Making new subfolder " << sensorfolder << endl;
			gDirectory->mkdir(sensorfolder);
			gDirectory->cd(sensorfolder);
			it->second->Write();

			//gDirectory->ls();
			gDirectory->cd("..");
		}
		gDirectory->cd("..");
	}
}

void HitmapCollection::Calculate(const unsigned int currentEventNumber)
{
  if ((currentEventNumber > 10 && currentEventNumber % 1000*_reduce == 0))
	{
		std::map<SimpleStandardPlane,HitmapHistos*>::iterator it;
		for (it = _map.begin(); it != _map.end(); ++it)
		{
			//std::cout << "Calculating ..." << std::endl;
			it->second->Calculate(currentEventNumber/_reduce);
		}
	}
}

void HitmapCollection::Reset()
{
	std::map<SimpleStandardPlane,HitmapHistos*>::iterator it;
	for (it = _map.begin(); it != _map.end(); ++it)
	{
		(*it).second->Reset();
	}

}

void HitmapCollection::Fill(const SimpleStandardEvent &simpev)
{

	for (int plane = 0; plane < simpev.getNPlanes(); plane++) {
		const SimpleStandardPlane&  simpPlane = simpev.getPlane(plane);
		fillHistograms(simpPlane);
	}
}
HitmapHistos * HitmapCollection::getHitmapHistos(std::string sensor, int id)
{
	SimpleStandardPlane sp(sensor,id);
	return _map[sp];
}


void HitmapCollection::registerPlane(const SimpleStandardPlane &p) {
	HitmapHistos *tmphisto = new HitmapHistos(p,_mon);
	_map[p] = tmphisto;
	//std::cout << "Registered Plane: " << p.getName() << " " << p.getID() << std::endl;
	//PlaneRegistered(p.getName(),p.getID());
	if (_mon != NULL)
	{
		if (_mon->getOnlineMon()==NULL)
		{
			return; // don't register items
		}
		cout << "HitmapCollection:: Monitor running in online-mode" << endl;
		char tree[1024], folder[1024];
		sprintf(tree,"%s/Sensor %i/RawHitmap",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getHitmapHisto(), "COLZ",0);


		sprintf(folder,"%s",p.getName().c_str());
#ifdef DEBUG
		cout << "DEBUG "<< p.getName().c_str() <<endl;
		cout << "DEBUG "<< folder << " "<<tree<<  endl;
#endif
		_mon->getOnlineMon()->addTreeItemSummary(folder,tree);


		sprintf(tree,"%s/Sensor %i/Clustermap",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getClusterMapHisto(), "COLZ",0);
		if ((p.is_APIX) || (p.is_USBPIX) || (p.is_USBPIXI4))
		{
			sprintf(tree,"%s/Sensor %i/LVL1Distr",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getLVL1Histo());

			sprintf(tree,"%s/Sensor %i/LVL1Cluster",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getLVL1ClusterHisto());

			sprintf(tree,"%s/Sensor %i/LVL1Width",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getLVL1WidthHisto());

			sprintf(tree,"%s/Sensor %i/SingleTOT",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getTOTSingleHisto());

			sprintf(tree,"%s/Sensor %i/ClusterTOT",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getTOTClusterHisto());

			sprintf(tree,"%s/Sensor %i/ClusterWidthX",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getClusterWidthXHisto());

			sprintf(tree,"%s/Sensor %i/ClusterWidthY",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getClusterWidthYHisto());
		}
		if (p.is_DEPFET)
		{
			sprintf(tree,"%s/Sensor %i/SingleTOT",p.getName().c_str(),p.getID());
			_mon->getOnlineMon()->registerTreeItem(tree);
			_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getTOTSingleHisto());
		}

		sprintf(tree,"%s/Sensor %i/Clustersize",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getClusterSizeHisto());

		sprintf(tree,"%s/Sensor %i/NumHits",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getNHitsHisto());

		sprintf(tree,"%s/Sensor %i/NumBadHits",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getNbadHitsHisto());
		sprintf(tree,"%s/Sensor %i/NumHotPixels",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getNHotPixelsHisto());

		sprintf(tree,"%s/Sensor %i/NumClusters",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getNClustersHisto());

		sprintf(tree,"%s/Sensor %i/HitOcc",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getHitOccHisto(), "",1);

		sprintf(tree,"%s/Sensor %i/Hot Pixel Map",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getHotPixelMapHisto(), "COLZ",0);

		if (p.is_MIMOSA26)
		{
		  // setup histogram showing the number of hits per section of a Mimosa26
		sprintf(tree,"%s/Sensor %i/Hitmap Sections",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getHitmapSectionsHisto());
		sprintf(tree,"%s/Sensor %i/Pivot Pixel",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->registerTreeItem(tree);
		_mon->getOnlineMon()->registerHisto(tree,getHitmapHistos(p.getName(),p.getID())->getNPivotPixelHisto());

		}

		sprintf(tree,"%s/Sensor %i",p.getName().c_str(),p.getID());
		_mon->getOnlineMon()->makeTreeItemSummary(tree);

		if (p.is_MIMOSA26)
		{
			char mytree[4][1024];// holds the number of histogramms for each section, hardcoded, will be moved to vectors
			TH1I * myhistos[4];//FIXME hardcoded
//loop over all sections
			for (unsigned int section =0; section<_mon->mon_configdata.getMimosa26_max_sections(); section ++ )
			{
				sprintf(mytree[0],"%s/Sensor %i/Section %i/NumHits",p.getName().c_str(),p.getID(),section);
				sprintf(mytree[1],"%s/Sensor %i/Section %i/NumClusters",p.getName().c_str(),p.getID(),section);
				sprintf(mytree[2],"%s/Sensor %i/Section %i/ClusterSize",p.getName().c_str(),p.getID(),section);
				sprintf(mytree[3],"%s/Sensor %i/Section %i/HotPixels",p.getName().c_str(),p.getID(),section);
				myhistos[0]=getHitmapHistos(p.getName(),p.getID())->getSectionsNHitsHisto(section);
				myhistos[1]=getHitmapHistos(p.getName(),p.getID())->getSectionsNClusterHisto(section);
				myhistos[2]=getHitmapHistos(p.getName(),p.getID())->getSectionsNClusterSizeHisto(section);
				myhistos[3]=getHitmapHistos(p.getName(),p.getID())->getSectionsNHotPixelsHisto(section);
//loop over all histograms
				for (unsigned int nhistos=0; nhistos<4; nhistos++)
				{
					if (myhistos[nhistos]==NULL)
					{
						cout << section << " " << "is null" << endl;
					}
					else
					{
						_mon->getOnlineMon()->registerTreeItem(mytree[nhistos]);
						_mon->getOnlineMon()->registerHisto(mytree[nhistos],myhistos[nhistos]);
					}
				}


				sprintf(tree,"%s/Sensor %i/Section %i",p.getName().c_str(),p.getID(),section);
				_mon->getOnlineMon()->makeTreeItemSummary(tree); //make summary page
			}
		}

	}
}

