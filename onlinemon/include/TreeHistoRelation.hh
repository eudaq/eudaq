#include <vector>
#include <string>
#include <iostream>
#include "TGListTree.h"
#include <TH1.h>
class HistoTreeContainer;
//!There is only one histogram per TreeItem, but the Draw-Method will draw also all histograms in the children of the TreeItem using the NextSibling-Method, typically an object like this contains an histogram or has at least one child
class HistoItem {
  protected:
    TGListTreeItem* 	_treeItem;
    std::string 		_treeItemString;
    TH1* 			_histo;
    TH1*			_overviewHisto;
    std::string		_histoOptions;
    unsigned int		_histoLogOption;
    TCanvas*			_fCanvas;

    void calculateNumSubpads(const unsigned int no);

  public:
    //!Constructor
    HistoItem(const TGListTreeItem* tree, const std::string str) 
      : _treeItem(tree), _treeItemString(str), _histo(0), _overviewHisto(0), _histoOptions(""), _histoLogOptions(0), _fCanvas(0)
        //!Nothing to do for the destructor
        virtual ~HistoItem() {};

    //!Add histograms to the TreeItem
    void setHisto(const TH1* histo, const std::string opt = "", const unsigned int logOpt = 0);
    //!If the Draw-Methods draws more than one histo the parent treeitems needs to know which of them to show in an overview
    void setOverviewHisto(const TH1* histo) {_overviewHisto = histo;}
    //!Simple print the TreeItemString for debugging
    void print() {std::cout << _treeItemString << std::endl;}
    //! Draw the histograms belonging to that TreeItem
    void Draw();
    //! Set the Canvas where the histograms should be drawn to
    void setCanvas(TCanvas *canvas) {fCanvas = canvas;}
    //!Getter for the histo
    TH1* const getHisto() {return _histo;}
};

//! This is the container-class for all TreeItem and histogram relationships. The drawing mechanism should work from HistoItemClass itself, this class should help to convert the different objects instead of using maps
class HistoTreeContainer{
  protected:
    std::map<TGListTreeItem*,HistoItem>	_treeItem;
    std::map<TH1,HistoItem*>	_histoItem;

  public:
    //! Standard Constructor - not much to do
    HistoTreeContainer() : _treeItem(0), _histoItem(0);
    //! Getter for the HistoItems - overloaded argument is the TreeItem - use this to call Draw() from the TreeBrowser e.g.
    HistoItem const getHistoItem(TGListTreeItem* item) {return _treeItem[item];}
    //! Getter for the HistoItems - overloaded argument is the histogrampointer - use this to call Draw() from the Clicking for Histograms e.g.
    HistoItem* const getHistoItem(TH1* histo) {return _histoItem[histo];}
    //! Add method 
    void add(TGListTreeItem* treeItem, &HistoItem histoItem) {
      _treeItem[treeItem] = histoItem;
      _histoItem[histoItem.getHisto()] = histoItem;
    }
};
