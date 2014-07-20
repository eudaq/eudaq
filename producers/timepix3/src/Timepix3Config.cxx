#include "Timepix3Config.h"
#include "Timepix3ConfigErrorHandler.h"

using namespace xercesc;

Timepix3Config::Timepix3Config() {
}

void Timepix3Config::ReadXMLConfig( string configFileName ) {

  std::cout << "Timepix3 XML config file is: " << configFileName << std::endl;
  
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

  cout << endl;

}



/*
  <registers>
    <reg name="IBIAS_PREAMP_ON" value="128"/>
    <reg name="IBIAS_PREAMP_OFF" value="8"/>
    <reg name="VPREAMP_NCAS" value="128"/>
    <reg name="IBIAS_IKRUM" value="15"/>
    <reg name="VFBK" value="164"/>
    <reg name="VTHRESH" value="1182"/>
    <reg name="IBIAS_DISCS1_ON" value="128"/>
    <reg name="IBIAS_DISCS1_OFF" value="8"/>
    <reg name="IBIAS_DISCS2_ON" value="32"/>
    <reg name="IBIAS_DISCS2_OFF" value="8"/>
    <reg name="IBIAS_PIXELDAC" value="128"/>
    <reg name="IBIAS_TPBUFIN" value="128"/>
    <reg name="IBIAS_TPBUFOUT" value="128"/>
    <reg name="VTP_COARSE" value="128"/>
    <reg name="VTP_FINE" value="256"/>
    <reg name="GeneralConfig" value="0x00000048"/>
    <reg name="PllConfig" value="0x0000281e"/>
    <reg name="OutputBlockConfig" value="0x00007b01"/>
  </registers>
*/


/*
XMLDocument doc;
  doc.LoadFile( configFileName.c_str() ); // "/home/vertextb/SPIDR/software/trunk/python/ui/x.t3x"

  XMLNode* root = doc.FirstChild(); // <?xml version="1.0" ?>
  XMLNode* tpix3 = root->NextSibling(); // <Timepix3>

  for( XMLElement* elem = tpix3->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()) {
    /*
    string elemName = elem->Value();
    cout << "\tElement name = " << elemName << endl;

    for( XMLElement* elemChild = elem->FirstChildElement(); elemChild != NULL; elemChild = elemChild->NextSiblingElement()) {
      
      string elemChildName = elemChild->Value();
      cout << "\t\tElement child name = " << elemChildName << endl;

      const char* time = elemChild->Attribute( "time" ); 
      if( time!= NULL ) {
	cout << "\t\t\tThe chip was configured on: " << time << endl;
	m_time = time;
      }
      const char* user = elemChild->Attribute( "user" );
      if( user!= NULL ) {
	cout << "\t\t\tUsername: " << user << endl;
	m_user = user;
      }
      const char* host = elemChild->Attribute( "host" ); 
      if( host!= NULL ) {
	cout << "\t\t\tHostname: " << host << endl;
	m_host = host;
      }
      
      const char* name = elemChild->Attribute( "name" );
      if( name != NULL ) {
      if( name == "IBIAS_PREAMP_ON" ) {
      const char* val = elemChild->Attribute( "value" );
      cout << "IBIAS_PREAMP_ON = " << val << endl;
      }
      
      }
      }
    */
