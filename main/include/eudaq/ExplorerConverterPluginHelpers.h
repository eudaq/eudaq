#ifndef EXPLORER_CONVERTER_PLUGIN_HELPERS_H
#define EXPLORER_CONVERTER_PLUGIN_HELPERS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

#include "TCanvas.h"
#include "TMath.h"
#include "TF1.h"
#include "TFitResultPtr.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGraphErrors.h"
#include "TFile.h"

//
// use namespace instead of class
//


//-------------------------------------------------------------------------------------------------
// static class containing the properties of the Explorer-1 matrices used in that gain measurement
class Ex1Prop
{
public:
    static const int   n_pix;       // number of pixels in the complete matrix
    static const int   n_sec;       // number of sectors per pitch
    static const int   n_rst;       // different reset types (non-permanent and permanent)
    static const int   n_mem;       // different memories (mem1, mem2 and cds:=mem1-mem2)
    static const int   w[2];        // number of pixels per column/row for a given pitch
    static const int   p[2];        // pixel pitch in um
    static const int   n_pitch;     // number of different pitches
    static const float meas_offset; // offset voltage appllied to the op amp on the proximity
};

const int   Ex1Prop::n_pix = 11700;
const int   Ex1Prop::n_sec = 9;
const int   Ex1Prop::n_rst = 2;
const int   Ex1Prop::n_mem = 3;
const int   Ex1Prop::w[] = { 90, 60 };
const int   Ex1Prop::p[] = { 20, 30 };
const int   Ex1Prop::n_pitch = sizeof(Ex1Prop::p)/sizeof(int);
const float Ex1Prop::meas_offset = 1.8 / 2.; // circuit on proximity leads to a divided by 2


//-------------------------------------------------------------------------------------------------
Float_t ADC_counts_to_V(Float_t val, Bool_t apply_offset = kTRUE)
{
    // theorectical conversion:
    //// input (ADC counts/V)
    //// output (unitless)
    //// SRS
    //const float ADC_val_rng  = 4096.;     // 12bit
    //const float ADC_volt_rng = 2.;        // Vpp
    //const float gain         = -1.25*2.0; // circuit gain (SRS * Proximity)
    //return (apply_offset) ? val/ADC_val_rng*ADC_volt_rng/gain + Ex1Prop::meas_offset
    //    : val/ADC_val_rng*ADC_volt_rng/gain;

    // measured conversion from self-test hybrid, mean value of all 4 channels:
    return (apply_offset) ? (6101.8 - val)/4507.8
        : -val/4507.8;     
}

//-------------------------------------------------------------------------------------------------
Float_t V_to_ADC_counts(Float_t val, Bool_t apply_offset = kTRUE)
{
    // theorectical conversion
    //// input (ADC counts/V)
    //// output (unitless)
    //// SRS
    //const float ADC_val_rng  = 4096.;     // 12bit
    //const float ADC_volt_rng = 2.;        // Vpp
    //const float gain         = -1.25*2.0; // circuit gain (SRS * Proximity)
    //return (apply_offset) ? (val - Ex1Prop::meas_offset)*ADC_val_rng/ADC_volt_rng*gain
    //    : val*ADC_val_rng/ADC_volt_rng*gain;

    // measured conversion from self-test hybrid, mean value of all 4 channels
    return (apply_offset) ? 6101.8 - val*4507.8
        : -val*4507.8;     
}

//-------------------------------------------------------------------------------------------------
Float_t DAC_counts_to_V(Int_t val)
{
    // input (DAC counts)

    return val/4096.*5.;
}

//-------------------------------------------------------------------------------------------------
Float_t V_to_C(Float_t val)
{
    // converting a signal in volts into a capacitance assuming the signal to be a Fe-55 generating
    // 1640 electrons of 1.602e-19C

    return 1640*1.602e-19/val;
}

//-------------------------------------------------------------------------------------------------
bool read_list_file(std::string name, std::vector<float>* vec) {
    std::ifstream f(name.c_str());
    std::string l;
    if (f.is_open()) {
        while (f.good()) {
            getline(f, l);
            if (l.length() > 0) {
                vec->push_back(atof(l.c_str())); // C++11 => std::stod(l)
            }
        }
    }
    else return false;
    return true;
}

//-------------------------------------------------------------------------------------------------
int pix_pitch(int ipix) {
    return (ipix < Ex1Prop::w[0]*Ex1Prop::w[0]) ? Ex1Prop::p[0] : Ex1Prop::p[1];
}

//-------------------------------------------------------------------------------------------------
int pix_row(int ipix) {
    return (ipix < Ex1Prop::w[0]*Ex1Prop::w[0]) ?
        ipix % Ex1Prop::w[0] :
        (ipix - Ex1Prop::w[0]*Ex1Prop::w[0]) % Ex1Prop::w[1];
}

//-------------------------------------------------------------------------------------------------
int pix_col(int ipix) {
    return (ipix < Ex1Prop::w[0]*Ex1Prop::w[0]) ?
        ipix / Ex1Prop::w[0] :
        (ipix - Ex1Prop::w[0]*Ex1Prop::w[0]) / Ex1Prop::w[1];
}

//-------------------------------------------------------------------------------------------------
int pix_sec(int ipix) {
    int w = (ipix < Ex1Prop::w[0]*Ex1Prop::w[0]) ? Ex1Prop::w[0] : Ex1Prop::w[1];
    int r_sec = (pix_row(ipix)*3)/w;
    int c_sec = (pix_col(ipix)*3)/w;
    return r_sec+c_sec*3;
}

//-------------------------------------------------------------------------------------------------
std::string mem_str(int i) {
    switch(i) {
    case 0: return "mem1";  break;
    case 1: return "mem2";  break;
    case 2: return "cds";   break;
    default:return "error"; break;
    }
    return "error";
}

//-------------------------------------------------------------------------------------------------
std::string rst_str(int i) {
    switch(i) {
    case 0: return "perm-rst";     break;
    case 1: return "non-perm-rst"; break;
    default:return "error";        break;
    }
    return "error";
}

//-------------------------------------------------------------------------------------------------
int transpose_sec(int i) {
    return (i%3)*3+(i/3);
}

//-------------------------------------------------------------------------------------------------
int centre_pix_sec(int i_sec, int i_pitch) {
    // selecting the lower right of the center pixels
    // [ ][ ]
    // [ ][x]
    // starting with (r=0,c=0) top left, (r=0,c=89) top right and (r=89,c=89) at bottom right
    int offset = i_pitch*Ex1Prop::w[0]*Ex1Prop::w[0];
    int row = Ex1Prop::w[i_pitch]*(0.5+i_sec%3)/3.;
    int col = Ex1Prop::w[i_pitch]*(0.5+i_sec/3)/3.;
    return offset + col*Ex1Prop::w[i_pitch]+row;
}

//-------------------------------------------------------------------------------------------------
Int_t get_index(Double_t* x, Float_t val, Int_t n)
{
    for (Int_t i=0; i<n; ++i) {
        if ((x[i]<=val && x[i+1]>val) || (x[i]>val && x[i+1]<=val)) {
            if (x[i]-val < x[i+1]-val) return i;
            else return i+1;
        }
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------
Float_t calc_gain(Float_t* x, Float_t* y, Int_t index, Int_t mode = 0)
{
    switch(mode) {
    case 0:
        return (y[index+1]-y[index])/(x[index+1]-x[index]);
        break;
    case 1:
        return (y[index+1]-y[index-1])/(x[index+1]-x[index-1]);
        break;
    }
    return 0.;
}

//-------------------------------------------------------------------------------------------------
Float_t calc_inl(Double_t* x, Double_t* y, Int_t i_start, Int_t i_end, Float_t gain)
{
    Float_t expected_sig = gain*(x[i_end]-x[i_start]);
    Float_t real_sig     = y[i_end]-y[i_start];
    return (real_sig/expected_sig-1.);
}

//-------------------------------------------------------------------------------------------------
Float_t calc_dnl(Float_t x, Float_t x2)
{
    return TMath::Abs(TMath::Abs(x-x2)/x);
}

//-------------------------------------------------------------------------------------------------
void average(Int_t n, Float_t* y, Float_t* y_err, Int_t mode = 0, Int_t avg_width = 11)
{
    // mode 0: weighted mean
    //      1: unweighted mean

    Float_t* t     = new Float_t[n];
    Float_t* t_err = new Float_t[n];

    memcpy(t,     y,     sizeof(Float_t)*n);
    memcpy(t_err, y_err, sizeof(Float_t)*n);
    // zero out border values
    for (Int_t a=0; a<avg_width; ++a) {
        y[a]         = 0.;
        y_err[a]     = 0.;
        y[n-1-a]     = 0.;
        y_err[n-1-a] = 0.;
    }

    for (Int_t i=0; i<n-avg_width; ++i) {
        Float_t sum_of_weights = 0.;
        Float_t mean           = 0.;
        Float_t mean_sq        = 0.;
        for (Int_t a=0; a<avg_width; ++a) {
            Float_t w = (mode==0) ? 1./(t_err[i+a]*t_err[i+a]) : 1.;
            sum_of_weights += w;
            mean           += w*t[i+a];
            mean_sq        += w*t[i+a]*t[i+a];
        }
        mean    /= sum_of_weights;
        mean_sq /= sum_of_weights;
        y[i+avg_width/2]     = mean;
        y_err[i+avg_width/2] = (mode==0) ? TMath::Sqrt(1/sum_of_weights) :
            TMath::Sqrt(mean_sq-mean*mean);
    }
}

//-------------------------------------------------------------------------------------------------
float read_vrst(std::string file)
{
    std::ifstream f(file.c_str());
    std::string l;
    if (f.is_open()) {
        while (f.good()) {
            getline(f, l);
            if (l.length() > 0) {
                return atof(l.c_str()); // C++11 => std::stod(l)
            }
        }
    }
    else {
        std::cerr << "Couldn't read input file" << std::endl;
        return 0.0;
    }
    return 0.0;
}

//-------------------------------------------------------------------------------------------------
struct gain_fit_result {
    // gain and error obtained from the linear fit
    Float_t g_lf;
    Float_t e_lf;
    Float_t ndf_lf;
    Float_t chi2_lf;

    // offset, linear and quadratic gain factors and errors obtained in the quadratic fit
    Float_t g_qf;       // linear gain factor
    Float_t e_qf;       // inear gain factor error
    Float_t g2_qf;      // quadratic gain factor
    Float_t e2_qf;      // quadratic gain factor error
    Float_t off_qf;     // offset 
    Float_t eff_qf;     // offset error
    Float_t ndf_qf;
    Float_t chi2_qf;

    // offset, linear and cubic gain factors and errors obtained in the cubic fit
    Float_t g_cf;       // linear gain factor
    Float_t e_cf;       // inear gain factor error
    Float_t g2_cf;      // quadratic gain factor
    Float_t e2_cf;      // quadratic gain factor error
    Float_t g3_cf;      // cubic gain factor
    Float_t e3_cf;      // cubic gain factor error
    Float_t off_cf;     // offset 
    Float_t eff_cf;     // offset error
    Float_t ndf_cf;
    Float_t chi2_cf;

    // the fit results
    TFitResultPtr r_lf;
    TFitResultPtr r_qf;
    TFitResultPtr r_cf;
};

gain_fit_result* do_gain_fits(TGraphErrors* g, Float_t v_rst = 0.8, Float_t v_sig = 0.10) {
    if ((v_rst-v_sig)<0.51) {
        std::cout << "-->>>>> WARNING: fit range might be going to a to small value!!!, please check results" << std::endl;
        std::cout << g->GetName() << std::endl;
    }

    gain_fit_result* result = new gain_fit_result;

    TF1* f_lin  = new TF1("f_lin",  TString::Format("[0]+[1]*(x-%f)", 
                          v_rst-.5*v_sig),
                          v_rst-v_sig, v_rst);
    f_lin->SetLineWidth(1);
    TF1* f_quad = new TF1("f_quad", TString::Format("[0]+[1]*(x-%f)+[2]*(x-%f)*(x-%f)",
                          v_rst-.5*v_sig, 
                          v_rst-.5*v_sig, v_rst-.5*v_sig),
                          v_rst-v_sig, v_rst);
    f_quad->SetLineWidth(1);
    TF1* f_cube = new TF1("f_cube", TString::Format("[0]+[1]*(x-%f)+[2]*(x-%f)*(x-%f)+[3]*(x-%f)*(x-%f)*(x-%f)",
                          v_rst-.5*v_sig, 
                          v_rst-.5*v_sig, v_rst-.5*v_sig, 
                          v_rst-.5*v_sig, v_rst-.5*v_sig, v_rst-.5*v_sig),
                          v_rst-v_sig, v_rst);
    f_cube->SetLineWidth(1);

    result->r_lf    = g->Fit(f_lin, "QSR");
    result->g_lf    = f_lin->GetParameter(1);
    result->e_lf    = f_lin->GetParError(1);
    result->ndf_lf  = f_lin->GetNDF();
    result->chi2_lf = f_lin->GetChisquare();

    result->r_qf    = g->Fit(f_quad, "QSR");
    result->g_qf    = f_quad->GetParameter(1);
    result->e_qf    = f_quad->GetParError(1);
    result->g2_qf   = f_quad->GetParameter(2);
    result->e2_qf   = f_quad->GetParError(2);
    result->off_qf  = f_quad->GetParameter(0);
    result->eff_qf  = f_quad->GetParError(0);
    result->ndf_qf  = f_quad->GetNDF();
    result->chi2_qf = f_quad->GetChisquare();

    f_cube->SetParameters(f_quad->GetParameter(0), f_quad->GetParameter(1), f_quad->GetParameter(2), 0);
    result->r_cf    = g->Fit(f_cube, "QSR");
    //result->r_cf    = g->Fit(f_cube, "SR");
    result->g_cf    = f_cube->GetParameter(1);
    result->e_cf    = f_cube->GetParError(1);
    result->g2_cf   = f_cube->GetParameter(2);
    result->e2_cf   = f_cube->GetParError(2);
    result->g3_cf   = f_cube->GetParameter(3);
    result->e3_cf   = f_cube->GetParError(3);
    result->off_cf  = f_cube->GetParameter(0);
    result->eff_cf  = f_cube->GetParError(0);
    result->ndf_cf  = f_cube->GetNDF();
    result->chi2_cf = f_cube->GetChisquare();

    
    delete f_lin;  f_lin  = 0x0;
    delete f_quad; f_quad = 0x0;
    delete f_cube; f_cube = 0x0;

    return result;
}


//-------------------------------------------------------------------------------------------------
TH1D* dist_from_map(TH2D* h, Int_t x_low = 1, Int_t x_up = -1, Int_t y_low = 1, Int_t y_up = -1,
                    TString name = "")
{
    // add suffix which indicates, that the distribution is based on a subsection only
    name = (name.Length() == 0) ? TString::Format("dist_%s", h->GetName()) : name;

    TH1D* dist = new TH1D(name,
                          TString::Format(";%s;probability", h->GetZaxis()->GetTitle()),
                          1000, h->GetMinimum(), h->GetMaximum());
    dist->Sumw2();

    x_up = (x_up == -1) ? h->GetNbinsX() : x_up;
    y_up = (y_up == -1) ? h->GetNbinsY() : y_up;

    for (Int_t i_x = x_low; i_x <= x_up; ++i_x) {
        for (Int_t i_y = y_low; i_y <= y_up; ++i_y) {
            dist->Fill(h->GetBinContent(i_x, i_y));
        }
    }

    dist->Scale(1./dist->Integral());
    return dist;
}

//-------------------------------------------------------------------------------------------------
class GainCorrection
{
protected:
    TFile* fMapFile;
    TH2* fOffsetMap;
    TH2* fGainMap;
    TH2* fGain2Map;
    TH2* fGain3Map;
    TH2* fEffVresetMap;
    TH2* fVresetNPROutMap;

    TF1* fFit;

    float fVsig;

    int fPitch;
    int fMem;
    float fVbb;
    float fVrstRef;
    int fChan;

    float fOffset;
    float fGain;
    float fGain2;
    float fGain3;
    float fEffVrst;
    float fVrstNPROut;

    bool fMapsAreLoaded;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool LoadMaps() {
        // loaded the maps necessary to do the correction
        if (fEffVresetMap){
            delete fEffVresetMap;
            fEffVresetMap = 0x0;
        }
        if (fVresetNPROutMap){
            delete fVresetNPROutMap;
            fVresetNPROutMap = 0x0;
        }
        if (fOffsetMap) {
            delete fOffsetMap;
            fOffsetMap = 0x0;
        }
        if (fGainMap) {
            delete fGainMap;
            fGainMap = 0x0;
        }
        if (fGain2Map) {
            delete fGain2Map;
            fGain2Map = 0x0;
        }
        if (fGain3Map) {
            delete fGain3Map;
            fGain3Map = 0x0;
        }
        fMapFile->cd();
        fMapsAreLoaded = true; // all maps are good -> changed below in case of problems

        // effective reset voltage
        TString eff_vrst_map_name = TString::Format("eff_vrst_map_vbb%0.1f_vrst%.2f_p%d_%s_chan%d",
                                                    fVbb, fVrstRef, Ex1Prop::p[fPitch],
                                                    mem_str(fMem).c_str(), fChan);
        gDirectory->GetObject(eff_vrst_map_name, fEffVresetMap);
        if (!fEffVresetMap) {
            fMapsAreLoaded = false;
            std::cerr << "Couldn't load the corresponding vrst map: "
                      << eff_vrst_map_name.Data()
                      << std::endl;
        }
        // vrst at output for NPR 
        TString vrst_npr_out_map_name = TString::Format("vrst_npr_out_map_vbb%0.1f_vrst%.2f_p%d_%s_chan%d",
                                                    fVbb, fVrstRef, Ex1Prop::p[fPitch],
                                                    mem_str(fMem).c_str(), fChan);
        gDirectory->GetObject(vrst_npr_out_map_name, fVresetNPROutMap);
        if (!fVresetNPROutMap) {
            fMapsAreLoaded = false;
            std::cerr << "Couldn't load the corresponding vrst map: "
                      << vrst_npr_out_map_name.Data()
                      << std::endl;
        }

        // offset (linear term parameter p0)
        TString offset_map_name = TString::Format("off_map_vbb%0.1f_vrst%.2f_p%d_%s_chan%d",
                                             fVbb, fVrstRef, Ex1Prop::p[fPitch],
                                             mem_str(fMem).c_str(), fChan);
        gDirectory->GetObject(offset_map_name, fOffsetMap);
        if (!fOffsetMap) {
            fMapsAreLoaded = false;
            std::cerr << "Couldn't load the corresponding gain map: "
                      << offset_map_name.Data()
                      << std::endl;
        }

        // gain (linear term parameter p1)
        TString g_map_name = TString::Format("g_map_vbb%0.1f_vrst%.2f_p%d_%s_chan%d",
                                             fVbb, fVrstRef, Ex1Prop::p[fPitch],
                                             mem_str(fMem).c_str(), fChan);
        gDirectory->GetObject(g_map_name, fGainMap);
        if (!fGainMap) {
            fMapsAreLoaded = false;
            std::cerr << "Couldn't load the corresponding gain map: "
                      << g_map_name.Data()
                      << std::endl;
        }

        // gain correction (quadratic term parameter p2)
        TString g2_map_name = TString::Format("g2_map_vbb%0.1f_vrst%.2f_p%d_%s_chan%d",
                                              fVbb, fVrstRef, Ex1Prop::p[fPitch],
                                              mem_str(fMem).c_str(), fChan);
        gDirectory->GetObject(g2_map_name, fGain2Map);
        if (!fGain2Map) {
            fMapsAreLoaded = false;
            std::cerr << "Couldn't load the corresponding gain correction map: "
                      << g2_map_name.Data()
                      << std::endl;
        }
        
        // gain correction (cubic term parameter p3)
        TString g3_map_name = TString::Format("g3_map_vbb%0.1f_vrst%.2f_p%d_%s_chan%d",
                                              fVbb, fVrstRef, Ex1Prop::p[fPitch],
                                              mem_str(fMem).c_str(), fChan);
        gDirectory->GetObject(g3_map_name, fGain3Map);
        if (!fGain3Map) {
            fMapsAreLoaded = false;
            std::cerr << "Couldn't load the corresponding gain correction map: "
                      << g3_map_name.Data()
                      << std::endl;
        }

        // fit function
        if (fFit) {
            delete fFit;
            fFit = 0x0;
        }
        fFit  = new TF1("fit", "[0]+[1]*(x-[4])+[2]*(x-[4])*(x-[4])+[3]*(x-[4])*(x-[4])*(x-[4])", 0.5, 0.8);

        return fMapsAreLoaded;
    }
public:
GainCorrection()
    : fMapFile(0x0)
        , fOffsetMap(0x0)
        , fGainMap(0x0)
        , fGain2Map(0x0)
        , fGain3Map(0x0)
        , fEffVresetMap(0x0)
        , fVresetNPROutMap(0x0)
        , fFit(0x0)
        , fVsig(0.1)
        , fPitch(-1)
        , fMem(1)
        , fVbb(1.8)
        , fVrstRef(0.7)
        , fChan(-1)
        , fOffset(0.)
        , fGain(0.)
        , fGain2(0.)
        , fGain3(0.)
        , fEffVrst(0.)
        , fVrstNPROut(0.)
        , fMapsAreLoaded(false)
    {

    }
    bool SetDataFile(TString filename) {
        TCanvas *mydummycanvas=new TCanvas();
        if (fMapFile) { delete fMapFile; fMapFile = 0x0; }
        fMapFile = new TFile(filename.Data());
        return (fMapFile && fMapFile->IsOpen());
    };
    void SetPitch(int p)       { fPitch   = p;        fMapsAreLoaded = false; };
    void SetVbb(float vbb)     { fVbb     = vbb-0.01; fMapsAreLoaded = false; }; // nasty trick to achieve -0.0
    void SetVrstRef(float vrr) { fVrstRef = vrr;      fMapsAreLoaded = false; };
    void SetMem(int mem)       { fMem     = mem;      fMapsAreLoaded = false; };
    void SetVsig(float vsig)   { fVsig    = vsig;     fMapsAreLoaded = false; };
    void SetChan(int chan)     { fChan    = chan;     fMapsAreLoaded = false; };

    float CorrectValue(float value, int i_row, int i_col, bool woOffset = true) {
        if (!fMapsAreLoaded) LoadMaps();
        if (!fMapsAreLoaded) {
            std::cerr << "Failed to load the corresponding map!" << std::endl;
            return -1.;
        }
        if (i_row < 0 || i_row >= Ex1Prop::w[fPitch]) {
            std::cerr << "row index out of bounds" << std::endl;
            return -1;
        }
        if (i_col < 0 || i_col >= Ex1Prop::w[fPitch]) {
            std::cerr << "column index out of bounds" << std::endl;
            return -1;
        }
        fOffset     = fOffsetMap->GetBinContent(i_col+1, i_row+1);
        fGain       = fGainMap->GetBinContent(i_col+1, i_row+1);
        fGain2      = fGain2Map->GetBinContent(i_col+1, i_row+1);
        fGain3      = fGain3Map->GetBinContent(i_col+1, i_row+1);
        fEffVrst    = fEffVresetMap->GetBinContent(i_col+1, i_row+1);
        fVrstNPROut = fVresetNPROutMap->GetBinContent(i_col+1, i_row+1);

        // woOffset assumes the value to be relative to the currently selected memory
        // for the explorer this should be the
        // please note that this is not necessarily correct due to the CDS shifting the dynamic
        // range
        
        // old: has issue that val is output val, i.e. misses gain correction, but it enters on Vrst axis in formula
        ///value = ADC_counts_to_V(value, !woOffset);
        ///value = (woOffset) ?
        ///    value/(fGain+2*fGainCorr*(value+.5*fVsig)):
        ///    value/(fGain+2*fGainCorr*(value-fEffVrst+.5*fVsig));
        ///return V_to_ADC_counts(value, !woOffset);
        
        // jacobus 20.10.2014
        // see his notebook for formula changes
        //if (TMath::Abs(value)>50 && fChan==2) {
        //    std::cout << "------ row: " << i_row  << ", col: " << i_col << " -----" << std::endl;
        //    std::cout << value << std::endl;
        //    std::cout << ADC_counts_to_V(value, !woOffset) << std::endl;
        //    std::cout << fOffset << std::endl;
        //    std::cout << fGain << std::endl;
        //    std::cout << fGainCorr << std::endl;
        //    std::cout <<    value*(fGain+fGainCorr*(-ADC_counts_to_V(value, !woOffset)+fVsig)) << std::endl;
        //    //std::cout <<    fOffset + fGain*(value-fVrstNPROut+0.5*fVsig) + fGainCorr*(value-fVrstNPROut+0.5*fVsig)*(value-fVrstNPROut+0.5*fVsig) << std::endl;
        //    std::cout << fVrstNPROut << std::endl;
        //    std::cout << fVsig << std::endl;
        //}

        value = ADC_counts_to_V(value, !woOffset);

        // quadratic fit with start of fit at fVrstNPROut
        //value = (woOffset) ?
        //    value*(fGain+fGain2*(-value+fVsig)):
        //    fOffset + fGain2*(value-fVrstNPROut+0.5*fVsig) + fGain2*(value-fVrstNPROut+0.5*fVsig)*(value-fVrstNPROut+0.5*fVsig)
        //         + fGain3*(value-fVrstNPROut+0.5*fVsig)*(value-fVrstNPROut+0.5*fVsig)*(value-fVrstNPROut+0.5*fVsig);
        
        // cubic fit
        Float_t vref =  fVrstNPROut+0.01-fVsig/2;
        fFit->SetParameters(fOffset, fGain, fGain2, fGain3, vref);
        value = (woOffset) ?
            fFit->Eval(fVrstNPROut)-fFit->Eval(fVrstNPROut-value):
            fFit->Eval(value);

        return V_to_ADC_counts(value, !woOffset);

    }
};

#endif
