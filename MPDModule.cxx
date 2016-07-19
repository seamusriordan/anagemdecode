/////////////////////////////////////////////////////////////////////
//
//   MPDModule
//   This is the MPD module decoder; based on SkeletonModule
//   (https://github.com/JeffersonLab/analyzer)
//
//   E. Cisbani
//   Original Version:   2015/Dec
//
/////////////////////////////////////////////////////////////////////

#define MPD_VERSION_TAG 0xE0000000
#define MPD_EVENT_TAG   0x10000000
#define MPD_MODULE_TAG  0x20000000
#define MPD_ADC_TAG     0x30000000
#define MPD_HEADER_TAG  0x40000000
#define MPD_DATA_TAG    0x0
#define MPD_TRAILER_TAG 0x50000000

#include "MPDModule.h"
#include "THaSlotData.h"

using namespace std;

namespace Decoder {

  Module::TypeIter_t MPDModule::fgThisType =
    DoRegister( ModuleType( "Decoder::MPDModule" , 3561 ));

  MPDModule::MPDModule(Int_t crate, Int_t slot) : VmeModule(crate, slot) {
    fDebugFile=0;
    Init();
  }
  
  MPDModule::~MPDModule() {
    
  }
  
  void MPDModule::Init() { 
    Module::Init();
    Config(0,25,6,16,128); // should be called by the user
    fDebugFile=0;
    Clear("");
//    fName = "MPD Module (INFN MPD for GEM and more), use Config to dynamic config";
    fName = "";
  }
  
#ifdef LIKEV792x
  Int_t MPDModule::LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, const UInt_t *pstop) {
    // This is a simple, default method for loading a slot
    const UInt_t *p = evbuffer;
    fWordsSeen = 0;
    //  cout << "version like V792"<<endl;
    ++p;
    Int_t nword=*p-2;
    ++p;
    for (Int_t i=0;i<nword;i++) {
      ++p;
      UInt_t chan=((*p)&0x00ff0000)>>16;
      UInt_t raw=((*p)&0x00000fff);
      Int_t status = sldat->loadData("adc",chan,raw,raw);
      fWordsSeen++;
      if (chan < fData.size()) fData[chan]=raw;
      //       cout << "word   "<<i<<"   "<<chan<<"   "<<raw<<endl;
      if( status != SD_OK ) return -1;
    }
    return fWordsSeen;
  }
#endif
  
  Int_t MPDModule::GetData(Int_t adc, Int_t sample, Int_t chan) const {
    Int_t idx = asc2i(adc, sample, chan);
    if ((idx < 0 ) || (idx >= fNumChan*fNumSample*fNumADC)) { return 0; }
    return fData[idx];
  }
  
  void MPDModule::Clear(const Option_t *opt) {
    fNumHits = 0;
    for (Int_t i=0; i<fNumChan*fNumSample*fNumADC; i++) fData[i]=0;
    for (Int_t i=0; i<fNumADC*fNumSample; i++) { 
      fFrameHeader[i]=0;
      fFrameTrailer[i]=0;
    }
    
  }
  
  Int_t MPDModule::Decode(const UInt_t *pdat) {
    
    UInt_t val = *pdat;
    UInt_t tag;
    
    Int_t adc;
    Int_t header;
    Int_t vdata;
    Int_t trailer;
    Int_t ch;
    
    static int bcount = 0;
    static int dcount = 0;
    static int version = 1;

    exit(1);
    
    tag = val & 0xF0000000;
    
    //  if (version == -1) { // first line
    //    version = 0;
    
    //    if (tag == MPD_VERSION_TAG) {
    //      version = (val &0xffff); 
    //      printf("File version = %x\n",version);
    //      continue; 
    //    }
    //  } 
    
    if (version>0) {
      switch (tag) {
	// case MPD_EVENT_TAG: // not in coda output
	// printf("#EVENT = %d\n", val&0xfffffff);
	// break;
      case MPD_MODULE_TAG:
	fIdxMPD = val&0xffff; // should be always the same
	printf(" MPD   = %d\n", val&0xffff);
	break;
      case MPD_ADC_TAG:
	adc = val&0xff;
	printf("   ADC = %d\n", adc); 
	fIdxA = adc;
	fIdxS = 0;
	break;
      case MPD_HEADER_TAG:
	header = (val >> 4)& 0x1ff; // truncate 111 bit at the beginning
	printf("    FRAME-HEADER %d (block=%d)", header, bcount);
	if ((header & 0xE00) == 0xE00) {
	  printf(" >>>\n");
	} else {
	  printf(" APV internal memory error in header decoding >>>\n");
	}
	fFrameHeader[as2i(fIdxA,fIdxS)] = header;
	break;
      case MPD_DATA_TAG:
	vdata = val&0xfff;
	ch = (val >> 12) & 0x7f;
	//      fData[asc2i(fIdxA,fIdxS,fIdxC)] = vdata;
	fData[asc2i(fIdxA,fIdxS,ch)] = vdata;
	fIdxC++;
	printf(" %d", vdata);
	if ((dcount % 16) == 15) { printf("\n"); }
	dcount++;
	break;
      case MPD_TRAILER_TAG:
	trailer = val & 0xfff; // *** TO BE VERIFIED ***
	fFrameTrailer[as2i(fIdxA,fIdxS)] = trailer;
	if ((dcount % 16) != 0) { printf(" >> missing data ? \n"); }
	dcount = 0;
	printf("    FRAME-TRAILER, SAMPLE index=%d <<<\n", trailer);
	bcount++;
	fIdxS++;
	fIdxC=0;
	break;
      default:
	printf("WARNING: wrong tag 0x%x\n",tag);
	break;
      }
    }
    
  }

}

ClassImp(Decoder::MPDModule)
