#include "Timepix3Config.h"
#include "Timepix3ConfigErrorHandler.h"

using namespace xercesc;

Timepix3Config::Timepix3Config() {
}

void Timepix3Config::ReadXMLConfig( string configFileName ) {

  std::cout << "Timepix3Config" << std::endl;
  std::cout << "Timepix3 XML config file is: " << configFileName << std::endl;
  std::cout << "reading XML..." << std::endl;
  
  // Initialize xerces
  try { XMLPlatformUtils::Initialize(); }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode( toCatch.getMessage() );
    cout << "Error during initialization! :\n" << message << endl;
    XMLString::release( &message );
    return;
  }
  
  //cout << "Xerces initialized." << endl;
  
  // Create parser, set parser values for validation
  XercesDOMParser* parser = new XercesDOMParser();
  parser->setValidationScheme( XercesDOMParser::Val_Never );
  parser->setDoNamespaces( false );
  parser->setDoSchema( false );
  parser->setValidationConstraintFatal( false );
  
  ErrorHandler* errHandler = (ErrorHandler*) new Timepix3ConfigErrorHandler();
  parser->setErrorHandler( errHandler );

  parser->parse( XMLString::transcode( configFileName.c_str() ) );
    
  DOMDocument* doc;
  try {
    doc = parser->getDocument();
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode( toCatch.getMessage() );
    cout << "Error getting document! :\n" << message << endl;
    XMLString::release( &message );
    return;
  }
  catch (const SAXParseException& toCatch) {
    char* message = XMLString::transcode( toCatch.getMessage() );
    cout << "Error getting document! :\n" << message << endl;
    XMLString::release( &message );
    return;
  }
  char* docURI = XMLString::transcode( doc->getDocumentURI() );
  //cout << "Got document: " << docURI << endl;

  DOMElement* docRootNode;
  try {
    docRootNode = doc->getDocumentElement();
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode( toCatch.getMessage() );
    cout << "Error getting root element! :\n" << message << endl;
    XMLString::release( &message );
    return;
  }
  const XMLCh* rootNodeName = docRootNode->getTagName();
  //cout << "Root node element name: " << XMLString::transcode( rootNodeName ) << endl;

  // Create the node iterator, that will walk through each element.
  DOMNodeIterator* walker;
  try {
    walker = doc->createNodeIterator( docRootNode, DOMNodeFilter::SHOW_ELEMENT, NULL, true );
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode( toCatch.getMessage() );
    cout << "Error creating node iterator! :\n" << message << endl;
    XMLString::release( &message );
    return;
  }  
  //cout << "Node iterator created." << endl;

  DOMNode * current_node = NULL;
  string thisNodeName;
  string parentNodeName;
  
  // Traverse the file
  for( current_node = walker->nextNode(); current_node != 0; current_node = walker->nextNode() ) {
    
    thisNodeName = XMLString::transcode( current_node->getNodeName() );
    parentNodeName = XMLString::transcode( current_node->getParentNode()->getNodeName() );

    DOMElement* current_element = dynamic_cast<DOMElement*>(current_node);

    if ( thisNodeName == "time" ) {
      m_time = XMLString::transcode( current_element->getAttribute( XMLString::transcode( "time" ) ) );
      //cout << "Config creation time: " << m_time << endl;
    }
    else if( thisNodeName == "user" ) {
      m_user = XMLString::transcode( current_element->getAttribute( XMLString::transcode( "user" ) ) );
      //cout << "Username: " << m_user << endl;
    }
    else if( thisNodeName == "host" ) {
      m_host = XMLString::transcode( current_element->getAttribute( XMLString::transcode( "host" ) ) );
      //cout << "Hostname: " << m_host << endl;
    }
    else if( thisNodeName == "reg" ) {
      string regName = XMLString::transcode( current_element->getAttribute( XMLString::transcode( "name" ) ) );
      TString regVal  = XMLString::transcode( current_element->getAttribute( XMLString::transcode( "value" ) ) );
      if( regVal.BeginsWith( "0x" ) ) {
	TString newRegVal = TString::BaseConvert( regVal, 16, 10 ); // convert hex to dec
	m_dacs[regName] = newRegVal.Atoi();
      } else {
	m_dacs[regName] = regVal.Atoi();
      }
      //cout << regName << " " << regVal << endl;
    }
    else if( thisNodeName == "codes" ) {
      TString codes = XMLString::transcode( current_node->getTextContent() );
      vector<TString> vrows = tokenise( codes, "\n" );
      for( int x = 0; x < vrows.size(); x++ ) {
	vector<TString> str_code = tokenise( vrows[x], " " );
	vector< int > tmpvec;
	for( int y = 0; y < str_code.size(); y++ ) {
	  int threshold = str_code[y].Atoi();
	  tmpvec.push_back( threshold );
	  //cout << x << " " << y << " " << threshold << endl;
	}
	m_matrixDACs.push_back( tmpvec );
      }
    }
    else if( thisNodeName == "mask" ) {
      TString mask = XMLString::transcode( current_node->getTextContent() );
      vector<TString> vrows = tokenise( mask, "\n" );
      for( int x = 0; x < vrows.size(); x++ ) {
	vector<TString> str_mask = tokenise( vrows[x], " " );
	vector< bool > tmpvec;
	for( int y = 0; y < str_mask.size(); y++ ) {
	  bool pixel_mask = str_mask[y].Atoi();
	  tmpvec.push_back( pixel_mask );
	  //cout << x << " " << y << " " << pixel_mask << endl;
	}
	m_matrixMask.push_back( tmpvec );
      }
    }
  }
}

void Timepix3Config::unpackGeneralConfig( int config ) {

  // Bits are defined in Table 18 of the Timepix3 manual v1.9

  typedef std::bitset<64> IntBits;
  
  if( IntBits( config ).test( 0 ) ) {
    cout << "\tPolarity is set to h+" << endl;
  } else {
    cout << "\tPolarity is set to e-" << endl;
  }

  if( IntBits( config ).test( 1 ) == false && IntBits( config ).test( 2 ) == false ) {
    cout << "\tOperation mode: ToT + ToA" << endl;
  } else if( IntBits( config ).test( 1 ) == true && IntBits( config ).test( 2 ) == false ) {
    cout << "\tOperation mode: ToA" << endl;
  } else if( IntBits( config ).test( 1 ) == false && IntBits( config ).test( 2 ) == true ) {
    cout << "\tOperation mode: Event + iToT" << endl;
  }

  if( IntBits( config ).test( 3 ) ) {
    cout << "\tGray Counter is ON" << endl;
  } else {
    cout << "\tGray Counter is OFF" << endl;
  }
  
  if( IntBits( config ).test( 4 ) ) {
    cout << "\tAckCommand is ON" << endl;
  } else {
    cout << "\tAckCommand is OFF" << endl;
  }

  if( IntBits( config ).test( 5 ) ) {
    cout << "\tTest Pulses are ON" << endl;
  } else {
    cout << "\tTest Pulses are OFF" << endl;
  }

  if( IntBits( config ).test( 6 ) ) {
    cout << "\tFast_lo_en is ON" << endl;
  } else {
    cout << "\tFast_lo_en is OFF" << endl;
  }

  if( IntBits( config ).test( 7 ) ) {
    cout << "\tTimerOverflowControl is ON" << endl;
  } else {
    cout << "\tTimerOverflowControl is OFF" << endl;
  }

  if( IntBits( config ).test( 9 ) ) {
    cout << "\tSending Digital TP [1]" << endl;
  } else {
    cout << "\tSending Analog TP [0] " << endl;
  }

  if( IntBits( config ).test( 10 ) ) {
    cout << "\tExtrenal TP [1]" << endl;
  } else {
    cout << "\tInternal TP [0] " << endl;
  }

  cout << endl;
}

string Timepix3Config::getChipID( int deviceID ) {

  if ( deviceID == 0 ) return "--";

  int x, y, w, mod, mod_val;
  x = deviceID & 0xF;
  y = ( deviceID >> 4 ) & 0xF;
  w = ( deviceID >> 8 ) & 0xFFF;
  mod = ( deviceID >> 20 ) & 0x3;
  mod_val = ( deviceID >> 22 ) & 0x3FF;
  
  if( mod == 1 ) {
    x = mod_val & 0xF;
  } else if( mod == 2 ) {
    y = mod_val & 0xF;
  } else if( mod == 3 ) {
    w = w & ~(0x3FF);
    w |= ( mod_val & 0x3FF );
  }
  char xs = (char) ( 64 + x );

  std::ostringstream oss;
  oss << "W" << w << "_" << xs << y;
  string chipID = oss.str();
  
  return chipID;
}
