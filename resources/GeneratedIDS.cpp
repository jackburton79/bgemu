/*
 * GeneratedIDS.cpp
 *
 *  Created on: 11/mag/2013
 *      Author: stefano
 */


#include "GeneratedIDS.h"
#include "IDSResource.h"

class WriteableIDSResource : public IDSResource {
public:
	WriteableIDSResource(const res_ref& name);
	bool AddValue(uint32 id, std::string value);
};

IDSResource*
GeneratedIDS::CreateIDSResource(const res_ref& name)
{
	WriteableIDSResource* res = new WriteableIDSResource(name);

	if (name == "ANISND")
		FillAniSnd(res);

	return res;
}


void
GeneratedIDS::FillAniSnd(WriteableIDSResource* res)
{
	res->AddValue(0x1000, "MWYV"); // Wyvern

	res->AddValue(0x2000, "MSIR");

	res->AddValue(0x3000, "MAKH"); // Ankheg

	res->AddValue(0x4400, "CHMW"); // Sleeping man TODO:

	// Character animations
	res->AddValue(0x5000, "CHMW");
	res->AddValue(0x5001, "CEMW");
	res->AddValue(0x5002, "CDMW");
	res->AddValue(0x5003, "CIMW");

	//res->AddValue(0x5010, "CHFB");

	res->AddValue(0x5011, "CEFB");
	res->AddValue(0x5012, "CDMW");
	res->AddValue(0x5013, "CIFB");
	res->AddValue(0x5100, "CHMB");
	res->AddValue(0x5101, "CEMW");
	res->AddValue(0x5102, "CDMF");	// Fighter Male Dwarf
	res->AddValue(0x5103, "CIMF");	// Fighter Male Halfling
	//res->AddValue(0x5110, "CHFB");
	res->AddValue(0x5111, "CEFB");
	res->AddValue(0x5112, "CDMW");
	res->AddValue(0x5113, "CIFB");
	res->AddValue(0x5200, "CHMW");
	res->AddValue(0x5201, "CEMW");
	res->AddValue(0x5202, "CDMW");
	res->AddValue(0x5210, "CHFW");
	res->AddValue(0x5211, "CEFW");
	res->AddValue(0x5212, "CDMW");
	res->AddValue(0x5300, "CHMB");
	res->AddValue(0x5301, "CEMB");
	res->AddValue(0x5302, "CDMW");
	res->AddValue(0x5303, "CIMB");
	//res->AddValue(0x5310, "CHFB");
	res->AddValue(0x5311, "CEFB");
	res->AddValue(0x5312, "CDMW");
	res->AddValue(0x5313, "CIFB");

	res->AddValue(0x6000, "CHMC"); // Cleric Male Human
	res->AddValue(0x6001, "CEMW");
	res->AddValue(0x6002, "CDMW");
	res->AddValue(0x6003, "CIMB");
	res->AddValue(0x6004, "CDMW");
	//res->AddValue(0x6005, "CHMB");
	//res->AddValue(0x6010, "CHFB");
	res->AddValue(0x6011, "CEFB");
	res->AddValue(0x6012, "CDMW");
	res->AddValue(0x6013, "CIFB");
	res->AddValue(0x6014, "CIFB");
	//res->AddValue(0x6015, "CHFB");
	res->AddValue(0x6100, "CHMF"); // Fighter Male Human
	res->AddValue(0x6101, "CEMB");
	res->AddValue(0x6102, "CDMF"); // Fighter Male Dwarf
	res->AddValue(0x6103, "CIMB");
	res->AddValue(0x6104, "CDMW");
	res->AddValue(0x6105, "CHMB");
	res->AddValue(0x6110, "CHFF"); // ?
	res->AddValue(0x6111, "CEFB");
	res->AddValue(0x6112, "CDMW");
	res->AddValue(0x6113, "CIFB");
	res->AddValue(0x6114, "CDFB");
	// res->AddValue(0x6115, "CHFB"); // ?
	res->AddValue(0x6200, "CHMW"); // Mage Male Human
	res->AddValue(0x6201, "CEMW");
	res->AddValue(0x6202, "CDMW");
	res->AddValue(0x6204, "CDMW");
	//res->AddValue(0x6205, "CHMW");
	res->AddValue(0x6210, "CHFW"); // Mage Female Human
	res->AddValue(0x6211, "CEFW");
	res->AddValue(0x6212, "CDMW");
	res->AddValue(0x6214, "CDMW");
	//res->AddValue(0x6215, "CHFW");
	res->AddValue(0x6300, "CHMT"); // Thief Human Male
	res->AddValue(0x6301, "CEMW");
	res->AddValue(0x6302, "CDMW");
	res->AddValue(0x6303, "CIMW");
	res->AddValue(0x6304, "CDMW");
	//res->AddValue(0x6305, "CHMB");
	//res->AddValue(0x6310, "CHFB");
	res->AddValue(0x6311, "CEFB");
	res->AddValue(0x6312, "CDMW");
	//res->AddValue(0x6313, "CIFB");
	//res->AddValue(0x6314, "CIFB");
	//res->AddValue(0x6315 "CHFB");

	// rest
	res->AddValue(0x6401, "UELM"); // ELMINSTER
	res->AddValue(0x6402, "USAR");
	res->AddValue(0x6403, "MSKL"); //CGAMEANIMATIONTYPE_SKELETON

	res->AddValue(0x6404, "USAR");

	res->AddValue(0x7000, "MOGR");	// HALF-OGRE
	res->AddValue(0x7001, "MOGR");	// OGRE

	res->AddValue(0x7400, "MDOG");
	res->AddValue(0x7600, "METT"); // ETTERCAP
	res->AddValue(0x7a00, "MSPI"); // SPIDER
	res->AddValue(0x7a02, "MSPI"); // PHASE_SPIDER
	res->AddValue(0x7a03, "MSPI"); // SWORD_SPIDER
	res->AddValue(0x7a04, "MSPI"); // SPIDER_WRAITH
	res->AddValue(0x7b00, "MWLF"); // WOLF
	res->AddValue(0x7b01, "MWLF"); // WOLF_WORG ?
	res->AddValue(0x7b05, "MWLF"); // WOLF_DREAD ?

	res->AddValue(0x7c01, "MTAS");
	res->AddValue(0x7d00, "MZOM"); // ZOMBIE
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

	res->AddValue(0x8000, "MGNL"); // Gnoll
	res->AddValue(0x8100, "MHOB");
	res->AddValue(0x9000, "MOGR");

	res->AddValue(0xa000, "MWYV"); // Baby Wyvern

	res->AddValue(0xb000, "ACOW"); // Cow
	res->AddValue(0xb100, "AHRS"); // Horse

	res->AddValue(0xb200, "NBEG");
	res->AddValue(0xb210, "NPRO");
	res->AddValue(0xb300, "NBOY");
	res->AddValue(0xb310, "NGRL");
	res->AddValue(0xb400, "NFAM");
	res->AddValue(0xb410, "NFAW");

	res->AddValue(0xb510, "NFAW"); // TODO

	res->AddValue(0xca00, "NNOM");
	res->AddValue(0xca10, "NNOW");

	res->AddValue(0xc100, "ACAT"); // Cat
	res->AddValue(0xc200, "ACHK"); // Chicken
	res->AddValue(0xc300, "ARAT"); // Rat
	res->AddValue(0xc400, "ASQU"); // Squirrel
	res->AddValue(0xc500, "ABAT"); // Bat

	res->AddValue(0xc600, "NBEG");
	res->AddValue(0xc610, "NFAW"); // HARLOT_WOMAN

	res->AddValue(0xc600, "NBEG");
	res->AddValue(0xc700, "NBOY");
	res->AddValue(0xc710, "NGRL");
	res->AddValue(0xc800, "NFAM");
	res->AddValue(0xc810, "NFAW");
	res->AddValue(0xc900, "NSIM");
	res->AddValue(0xc910, "NFAW"); // TODO

	// Birds
	res->AddValue(0xd000, "AEAG"); // Eagle
	res->AddValue(0xd100, "AGUL"); // Seagul
	res->AddValue(0xd200, "AVUL");
	res->AddValue(0xd300, "ABIR"); // Bird

	res->AddValue(0xe010, "METT");
}


// WriteIDSResource
WriteableIDSResource::WriteableIDSResource(const res_ref& name)
	:
	IDSResource(name)
{
}


bool
WriteableIDSResource::AddValue(uint32 id, std::string value)
{
	fMap[id] = value;
	return true;
}
