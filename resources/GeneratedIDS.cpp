/*
 * GeneratedIDS.cpp
 *
 *  Created on: 11/mag/2013
 *      Author: stefano
 */


#include "GeneratedIDS.h"
#include "IDSResource.h"

class WriteIDSResource : public IDSResource {
public:
	WriteIDSResource(const res_ref& name);
	bool AddValue(uint32 id, std::string value);

};

IDSResource*
GeneratedIDS::CreateIDSResource(const res_ref& name)
{
	WriteIDSResource* res = new WriteIDSResource(name);

	if (name == "ANISND")
		FillAniSnd(res);

	return res;
}


void
GeneratedIDS::FillAniSnd(WriteIDSResource* res)
{
	res->AddValue(0x2000, "MSIR");

	// Character animations
	res->AddValue(0x5000, "CHMB");
	res->AddValue(0x5001, "CEMB");
	res->AddValue(0x5002, "CDMB");
	res->AddValue(0x5003, "CIMB");
	res->AddValue(0x5010, "CHFB");
	res->AddValue(0x5011, "CEFB");
	res->AddValue(0x5012, "CDMB");
	res->AddValue(0x5013, "CIFB");
	res->AddValue(0x5100, "CHMB");
	res->AddValue(0x5101, "CEMB");
	res->AddValue(0x5102, "CDMB");
	res->AddValue(0x5103, "CIMB");
	res->AddValue(0x5110, "CHFB");
	res->AddValue(0x5111, "CEFB");
	res->AddValue(0x5112, "CDMB");
	res->AddValue(0x5113, "CIFB");
	res->AddValue(0x5200, "CHMW");
	res->AddValue(0x5201, "CEMW");
	res->AddValue(0x5202, "CDMW");
	res->AddValue(0x5210, "CHFW");
	res->AddValue(0x5211, "CEFW");
	res->AddValue(0x5212, "CDMW");
	res->AddValue(0x5300, "CHMB");
	res->AddValue(0x5301, "CEMB");
	res->AddValue(0x5302, "CDMB");
	res->AddValue(0x5303, "CIMB");
	res->AddValue(0x5310, "CHFB");
	res->AddValue(0x5311, "CEFB");
	res->AddValue(0x5312, "CDMB");
	res->AddValue(0x5313, "CIFB");
	res->AddValue(0x6000, "CHMB");
	res->AddValue(0x6001, "CEMB");
	res->AddValue(0x6002, "CDMB");
	res->AddValue(0x6003, "CIMB");
	res->AddValue(0x6004, "CDMB");
	//res->AddValue(0x6005, "CHMB");
	res->AddValue(0x6010, "CHFB");
	res->AddValue(0x6011, "CEFB");
	res->AddValue(0x6012, "CDMB");
	res->AddValue(0x6013, "CIFB");
	res->AddValue(0x6014, "CIFB");
	//res->AddValue(0x6015, "CHFB");
	res->AddValue(0x6100, "CHMB");
	res->AddValue(0x6101, "CEMB");
	res->AddValue(0x6102, "CDMB");
	res->AddValue(0x6103, "CIMB");
	res->AddValue(0x6104, "CDMB");
	res->AddValue(0x6105, "CHMB");
	res->AddValue(0x6110, "CHFB");
	res->AddValue(0x6111, "CEFB");
	res->AddValue(0x6112, "CDMB");
	res->AddValue(0x6113, "CIFB");
	res->AddValue(0x6114, "CDFB");
	//res->AddValue(0x6115, "CHFB");
	res->AddValue(0x6200, "CHMW");
	res->AddValue(0x6201, "CEMW");
	res->AddValue(0x6202, "CDMW");
	res->AddValue(0x6204, "CDMW");
	//res->AddValue(0x6205, "CHMW");
	res->AddValue(0x6210, "CHFW");
	res->AddValue(0x6211, "CEFW");
	res->AddValue(0x6212, "CDMW");
	res->AddValue(0x6214, "CDMW");
	//res->AddValue(0x6215, "CHFW");
	res->AddValue(0x6300, "CHMB");
	res->AddValue(0x6301, "CEMB");
	res->AddValue(0x6302, "CDMB");
	res->AddValue(0x6303, "CIMB");
	res->AddValue(0x6304, "CDMB");
	//res->AddValue(0x6305, "CHMB");
	res->AddValue(0x6310, "CHFB");
	res->AddValue(0x6311, "CEFB");
	res->AddValue(0x6312, "CDMB");
	//res->AddValue(0x6313, "CIFB");
	//res->AddValue(0x6314, "CIFB");
	//res->AddValue(0x6315 "CHFB");

	// rest
	res->AddValue(0x6404, "USAR1");
	res->AddValue(0x7001, "MOGR");
	res->AddValue(0x7400, "MDOG");
	res->AddValue(0x7a00, "MSPI");
	res->AddValue(0x7c01, "MTAS");
	res->AddValue(0x7e00, "MWER");
	res->AddValue(0x7f01, "MMIN");
	res->AddValue(0x7f02, "MBEH");
	res->AddValue(0x7f03, "MIMP");
	res->AddValue(0x7f09, "MSAH");
	res->AddValue(0x7f0c, "MKUO");
	res->AddValue(0x7f11, "MUMB");
	res->AddValue(0x7f14, "MGIT");
	res->AddValue(0x7f15, "MBES");
	res->AddValue(0x7f16, "AMOO");
	res->AddValue(0x7f17, "ARAB");
	res->AddValue(0x7f18, "ADER");
	res->AddValue(0x7f20, "AGRO");
	res->AddValue(0x7f21, "APHE");
	res->AddValue(0x7f27, "MDRO");
	res->AddValue(0x7f28, "MKUL");

	res->AddValue(0x8000, "MGNL");
	res->AddValue(0x8100, "MHOB");
	res->AddValue(0x9000, "MOGR");
	res->AddValue(0xb000, "ACOW");
	res->AddValue(0xb100, "AHRS");

	res->AddValue(0xb200, "NBEG");
	res->AddValue(0xb210, "NPRO");
	res->AddValue(0xb300, "NBOY");
	res->AddValue(0xb310, "NGRL");
	res->AddValue(0xb400, "NFAM");
	res->AddValue(0xb410, "NFAW");

	res->AddValue(0xca00, "NNOM");
	res->AddValue(0xca10, "NNOW");

	res->AddValue(0xc100, "ACAT");
	res->AddValue(0xc200, "ACHK");
	res->AddValue(0xc300, "ARAT");
	res->AddValue(0xc400, "ASQU");
	res->AddValue(0xc500, "ABAT");

	res->AddValue(0xc600, "NBEG");
	res->AddValue(0xc700, "NBOY");
	res->AddValue(0xc710, "NGRL");
	res->AddValue(0xc800, "NFAM");
	res->AddValue(0xc810, "NFAW");
	res->AddValue(0xc900, "NFAM"); // TODO
	res->AddValue(0xc910, "NFAW"); // TODO

	// Birds
	res->AddValue(0xd000, "AEAG");
	res->AddValue(0xd100, "AGUL");
	res->AddValue(0xd200, "AVUL");
	res->AddValue(0xd300, "ABIR");

	res->AddValue(0xe010, "METT");
}


// WriteIDSResource
WriteIDSResource::WriteIDSResource(const res_ref& name)
	:
	IDSResource(name)
{
}


bool
WriteIDSResource::AddValue(uint32 id, std::string value)
{
	fMap[id] = value;
	return true;
}
