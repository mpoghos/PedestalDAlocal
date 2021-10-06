#if !defined(__CLING__) || defined(__ROOTCLING__)
#include "RStringView.h"
#include <Rtypes.h>
#include <TCanvas.h>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

#include "CCDB/CcdbApi.h"
#include "DetectorsRaw/RawFileReader.h"
#include "EMCALReconstruction/AltroDecoder.h"
#include "EMCALReconstruction/Bunch.h"
#include "EMCALReconstruction/CaloFitResults.h"
#include "EMCALReconstruction/CaloRawFitterStandard.h"

#include "EMCALBase/Geometry.h"
#include "EMCALBase/Mapper.h"

#include "CommonConstants/Triggers.h"

#include "DataFormatsEMCAL/Constants.h"

#endif

using namespace o2::emcal;

const int debug = -2;

std::array<TProfile2D *, 20> mHistoHG;
std::array<TProfile2D *, 20> mHistoLG;
std::array<TProfile *, 20> mHistoLEDHG;
std::array<TProfile *, 20> mHistoLEDLG;

o2::emcal::Geometry *geo;
std::unique_ptr<o2::emcal::MappingHandler> mMapper = nullptr;

using CHTYP = o2::emcal::ChannelType_t;

// some global var/constants
const Int_t kNSM = 20;  // number of SuperModules
const Int_t kNRCU = 2;  // number of readout crates (and DDLs) per SM
const Int_t kNDTC = 40; // links for full SRU
const Int_t kNBranch = 2;
const Int_t kNFEC = 10; // 0..9, when including LED Ref
const Int_t kNChip = 5; // really 0,2..4, i.e. skip #1
const Int_t kNChan = 16;
Float_t fMeanPed[kNSM][kNRCU][kNBranch][kNFEC][kNChip][kNChan];
Float_t fRmsPed[kNSM][kNRCU][kNBranch][kNFEC][kNChip][kNChan];
//
const int kNStrips = 24;          // per SM
Int_t fHWAddrLEDRef[kNStrips][2]; // [2] is for Low/High gain

const Float_t kBadRMS = 20;

void AnalRawData();
void AnalChannelData();
void WriteToCCDB();
void DrawHistos();
void Clear();
void CreateMappingLEDRef();
Int_t GetHWAddressLEDRef(Int_t istrip, Int_t igain);
Int_t GetHWAddress(Int_t iside, Int_t icol, Int_t irow, CHTYP igain,
                   Int_t &iRCU);
void PedValFEC();
void PedValLEDRef();
void FillTextFile(const char *filename);

int iDET = 0;
int runNumber = 0;

// iDet = 0 for EMCAL; 1 for DCAL
void AnalData2textFile(int run_number, int iDet = 0) {
  runNumber = run_number;

  std::cout << "iDet = " << iDet << endl;
  iDET = iDet;

  for (auto ism = 0; ism < 20; ism++) {
    mHistoHG[ism] =
        new TProfile2D(Form("mHistoHG_SM%d", ism), Form("mHistoHG_SM%d", ism),
                       48, -0.5, 47.5, 24, -0.5, 23.5);
    mHistoLG[ism] =
        new TProfile2D(Form("mHistoLG_SM%d", ism), Form("mHistoLG_SM%d", ism),
                       48, -0.5, 47.5, 24, -0.5, 23.5);
    mHistoLEDHG[ism] =
        new TProfile(Form("mHistoLEDHG_SM%d", ism),
                     Form("mHistoLEDHG_SM%d", ism), 24, -0.5, 23.5);
    mHistoLEDLG[ism] =
        new TProfile(Form("mHistoLEDLG_SM%d", ism),
                     Form("mHistoLEDLG_SM%d", ism), 24, -0.5, 23.5);
  }

  // Use the RawReaderFile to read the raw data file
  const char *aliceO2env = std::getenv("O2_ROOT");

  geo = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);

  if (!mMapper) {
    mMapper = std::unique_ptr<o2::emcal::MappingHandler>(
        new o2::emcal::MappingHandler);
  }
  if (!mMapper) {
    LOG(ERROR) << "Failed to initialize mapper";
  }

  AnalRawData();
  // AnalChannelData();

  // WriteToCCDB();
  /*
    for (int row = 0; row < 24; row++) {
      for (int col = 0; col < 32; col++) {
        mHistoHG[12]->Fill(col, row, 1);
        mHistoLG[12]->Fill(col, row, 1);
      }
      for (int col = 16; col < 48; col++) {
        mHistoHG[13]->Fill(col, row, 1);
        mHistoLG[13]->Fill(col, row, 1);
      }
    }
  */
  /*
    mHistoHG[12]->Fill(Double_t(0), Double_t(0), 10.);
    mHistoHG[12]->Fill(Double_t(1), Double_t(1), 10.);
    mHistoHG[12]->Fill(Double_t(2), Double_t(2), 10.);
    mHistoHG[12]->Fill(Double_t(3), Double_t(3), 10.);
    mHistoHG[12]->Fill(Double_t(4), Double_t(4), 10.);
    mHistoHG[12]->Fill(Double_t(5), Double_t(5), 10.);
    mHistoHG[12]->Fill(Double_t(6), Double_t(6), 10.);
    mHistoHG[12]->Fill(Double_t(7), Double_t(7), 10.);
  */

  Clear();
  CreateMappingLEDRef();

  PedValFEC();
  PedValLEDRef();

  if (iDET == 0) {
    FillTextFile(Form("pedestalData_%d_EMCAL.txt", runNumber));
  } else {
    FillTextFile(Form("pedestalData_%d_DCAL.txt", runNumber));
  }
  DrawHistos();
}

void AnalRawData() {
  //  using CHTYP = o2::emcal::ChannelType_t;

  o2::raw::RawFileReader reader;
  reader.setDefaultDataOrigin(o2::header::gDataOriginEMC);
  reader.setDefaultDataDescription(o2::header::gDataDescriptionRawData);
  reader.setDefaultReadoutCardType(o2::raw::RawFileReader::RORC);
  //  reader.addFile("data/readoutSMAC9_pedestal.raw");
  //  reader.addFile("data/readoutEMCAL.raw");
  if (iDET == 0) {
    reader.addFile(Form("data/readoutEMCAL_%d.raw", runNumber));
  } else {
    reader.addFile(Form("data/readoutDCAL_%d.raw", runNumber));
  }
  reader.init();

  /*
    std::unique_ptr<o2::emcal::MappingHandler> mMapper = nullptr;
    if (!mMapper) {
      mMapper = std::unique_ptr<o2::emcal::MappingHandler>(new
    o2::emcal::MappingHandler);
    }
    if (!mMapper) {
      LOG(ERROR) << "Failed to initialize mapper";
    }
  */

  while (1) {
    int tfID = reader.getNextTFToRead();
    if (tfID >= reader.getNTimeFrames()) {
      std::cerr << "nothing left to read after " << tfID << " TFs read";
      break;
    }
    std::vector<char> dataBuffer; // where to put extracted data
    for (int il = 0; il < reader.getNLinks(); il++) {
      auto &link = reader.getLink(il);

      if (debug > 2)
        std::cout << "Decoding link " << il << std::endl;

      auto sz = link.getNextTFSize(); // size in bytes needed for the next TF of
                                      // this link
      dataBuffer.resize(sz);
      link.readNextTF(dataBuffer.data());

      // Parse
      o2::emcal::RawReaderMemory parser(dataBuffer);

      while (parser.hasNext()) {
        parser.next();

        if (o2::raw::RDHUtils::getFEEID(parser.getRawHeader()) >= 40)
          continue;

        auto &header = parser.getRawHeader();
        auto triggerBC = o2::raw::RDHUtils::getTriggerBC(header);
        auto triggerOrbit = o2::raw::RDHUtils::getTriggerOrbit(header);
        auto feeID = o2::raw::RDHUtils::getFEEID(header);
        auto triggerbits = o2::raw::RDHUtils::getTriggerType(header);
        bool kIsPHYSICS = triggerbits & o2::trigger::PhT;

        if (!kIsPHYSICS)
          continue;

        if (debug > 1) {
          std::cout << "FEEID: " << feeID << std::endl;
          std::cout << "next page: (TriggerBits=" << triggerbits << ",  ";
          std::cout << "Orbit=" << triggerOrbit << ", BC=" << triggerBC << ")"
                    << std::endl;
        }

        o2::emcal::AltroDecoder decoder(parser);
        decoder.decode();

        int nchannels = 0;
        int iSM = feeID / 2;

        int feeID1 = feeID;
        /*
        if (iSM % 2 == 1) {
          if (feeID % 2 == 0)
            feeID1 = feeID + 1;
          else if (feeID % 2 == 1)
            feeID1 = feeID - 1;
        }
*/

        o2::emcal::Mapper map = mMapper->getMappingForDDL(feeID1);

        // Loop over all the channels
        for (auto &chan : decoder.getChannels()) {
          int row, col;
          ChannelType_t chType;
          //      CHTYP chType;
          try {
            row = map.getRow(chan.getHardwareAddress());
            col = map.getColumn(chan.getHardwareAddress());
            chType = map.getChannelType(chan.getHardwareAddress());

          } catch (Mapper::AddressNotFoundException &ex) {
            std::cerr << ex.what() << std::endl;
            continue;
          };

          if (chType == CHTYP::TRU)
            continue;

          //// Int_t absID = geo->GetAbsCellIdFromCellIndexes(iSM, row, col);
          // auto [rowOffline, colOffline] =
          // geo->ShiftOnlineToOfflineCellIndexes(iSM, row, col);
          // Int_t absID =
          // geo->GetAbsCellIdFromCellIndexes(iSM, rowOffline, colOffline);

          //          if (chType == CHTYP::LEDMON || chType == CHTYP::TRU)

          nchannels++;

          // if(!(iSM==12 && col == 0 && row == 0))
          //  continue;

          if (debug > 1)
            std::cout << "SM" << iSM << "/FEC" << chan.getFECIndex()
                      << "/HW:" << chan.getHardwareAddress() << "/CF:" << chType
                      << " (" << setw(2) << col << "," << setw(2) << row
                      << "): ";

          int nsamples = 0;

          for (auto &bunch : chan.getBunches()) {
            for (auto const e : bunch.getADC()) {
              if (debug > 1)
                std::cout << setw(4) << e << " ";
              nsamples++;

              if (chType == CHTYP::HIGH_GAIN)
                mHistoHG[iSM]->Fill(col, row, e);
              if (chType == CHTYP::LOW_GAIN)
                mHistoLG[iSM]->Fill(col, row, e);
              if (chType == CHTYP::LEDMON) {
                if (row == 0)
                  mHistoLEDLG[iSM]->Fill(col, e);
                else
                  mHistoLEDHG[iSM]->Fill(col, e);
              }
            }
            if (debug > 1)
              std::cout << std::endl;
          }
        }

        if (debug > 1)
          std::cout << "channels found : " << nchannels << std::endl;
      }
    }
    reader.setNextTFToRead(++tfID);
  }
}

/*
void WriteToCCDB() {

  std::cout << "writing to CCDB" << std::endl;

  o2::ccdb::CcdbApi api;
  const std::string uri = "http://ccdb-test.cern.ch:8080";
  api.init(uri); // or http://localhost:8080 for a local installation
  if (!api.isHostReachable()) {
    LOG(WARNING) << "Host " << uri << " is not reacheable, abandoning the test";
    return;
  }

  std::map<std::string, std::string> metadata;
  metadata["responsible"] = "Martin Poghosyan";
  api.storeAsTFileAny(ped.get(), "EMCAL/PEDESTAL", metadata, 9, 99999999999999);
}
*/
void DrawHistos() {
  /*
    cHG->cd(1);
    mHistoHG[8]->Draw("colz");
    cHG->cd(2);
    mHistoHG[9]->Draw("colz");

    return;
   */
  auto cHG = new TCanvas("cHG", "cHG");
  auto cLG = new TCanvas("cLG", "cLG");
  auto cLEDHG = new TCanvas("cLEDHG", "cLEDHG");
  auto cLEDLG = new TCanvas("cLEDLG", "cLEDLG");

  cHG->Divide(2, 10);
  cLG->Divide(2, 10);
  cLEDHG->Divide(2, 10);
  cLEDLG->Divide(2, 10);

  for (int iSM = 0; iSM < 20; iSM++) {
    cHG->cd(iSM + 1);
    mHistoHG[iSM]->Draw("colz");
    cLG->cd(iSM + 1);
    mHistoLG[iSM]->Draw("colz");

    cLEDHG->cd(iSM + 1);
    mHistoLEDHG[iSM]->Draw("colz");
    cLEDLG->cd(iSM + 1);
    mHistoLEDLG[iSM]->Draw("colz");
  }
}

void Clear() {
  for (Int_t iSM = 0; iSM < kNSM; iSM++) {
    for (Int_t iRCU = 0; iRCU < kNRCU; iRCU++) {
      for (Int_t ibranch = 0; ibranch < kNBranch; ibranch++) {
        for (Int_t iFEC = 0; iFEC < kNFEC; iFEC++) {
          for (Int_t ichip = 0; ichip < kNChip; ichip++) {
            for (Int_t ichan = 0; ichan < kNChan; ichan++) {
              fMeanPed[iSM][iRCU][ibranch][iFEC][ichip][ichan] = 0;
              fRmsPed[iSM][iRCU][ibranch][iFEC][ichip][ichan] = 0;
            }
          }
        }
      }
    }
  }

  for (int istrip = 0; istrip < kNStrips; istrip++) {
    fHWAddrLEDRef[istrip][0] = 0;
    fHWAddrLEDRef[istrip][1] = 0;
  }

  return;
}

void DecodeHWAddress(Int_t hwAddr, Int_t &branch, Int_t &FEC, Int_t &chip,
                     Int_t &chan) {
  chan = hwAddr & 0xf;
  chip = (hwAddr >> 4) & 0x7;
  FEC = (hwAddr >> 7) & 0xf;
  branch = (hwAddr >> 11) & 0x1;
  return;
}

void CreateMappingLEDRef() {
  Int_t feeID = 0; // for both sides; LED ref info is the same for both sides

  Int_t maxAddr = 1 << 7; // LED Ref FEE is in FEC pos 0, i.e. addr space 0..127

  int nLEDRefFEEChan = 0;

  Int_t branch = 0;
  Int_t FEC = 0;
  Int_t chip = 0;
  Int_t chan = 0;

  const auto &map = mMapper->getMappingForDDL(feeID);

  for (int hwaddr = 0; hwaddr < maxAddr; hwaddr++) {

    DecodeHWAddress(hwaddr, branch, FEC, chip, chan);
    if ((chip != 1 && chip < kNChip) && // ALTROs 0,2,3,4
        (chan < 8 || chan > 11)) {      // actual installed LED Ref FEE channels

      int istrip = map.getColumn(hwaddr);
      int igain = map.getRow(hwaddr);
      ChannelType_t chantype = map.getChannelType(hwaddr);

      if (chantype == CHTYP::LEDMON) {
        fHWAddrLEDRef[istrip][igain] = hwaddr;
        nLEDRefFEEChan++;
      }
    }
  }

  if (debug > 0) {
    cout << " nLEDRefFEEChan " << nLEDRefFEEChan << endl;
  }
}

void PedValFEC() {
  Int_t hwAddress = 0;
  Int_t iRCU = 0;
  Int_t branch = 0;
  Int_t FEC = 0;
  Int_t chip = 0;
  Int_t chan = 0;

  for (int iSM = 0; iSM < 20; iSM++) {
    Int_t isect = iSM / 2; //
    Int_t iside = iSM % 2; // A or C side

    for (int col = 0; col < 48; col++) {
      for (int row = 0; row < 24; row++) {
        hwAddress = GetHWAddress(iside, col, row, CHTYP::HIGH_GAIN, iRCU);
        DecodeHWAddress(hwAddress, branch, FEC, chip, chan);
        fMeanPed[iSM][iRCU][branch][FEC][chip][chan] = short(
            mHistoHG[iSM]->GetBinContent(col + 1, row + 1) + 0.5); // fix 0.5?

        hwAddress = GetHWAddress(iside, col, row, CHTYP::LOW_GAIN, iRCU);
        DecodeHWAddress(hwAddress, branch, FEC, chip, chan);
        fMeanPed[iSM][iRCU][branch][FEC][chip][chan] = short(
            mHistoLG[iSM]->GetBinContent(col + 1, row + 1) + 0.5); // fix 0.5?
      }
    }
  }

  return;
}

void PedValLEDRef() {

  for (Int_t iSM = 0; iSM < 20; iSM++) {
    Int_t isect = iSM / 2; //
    Int_t iside = iSM % 2; // A or C side
    Int_t nStrips = 24;

    Int_t hwAddress = 0;
    Int_t iRCU = 0; // always true for LED Ref FEE
    Int_t branch = 0;
    Int_t FEC = 0;
    Int_t chip = 0;
    Int_t chan = 0;

    Int_t icol = 0;
    Int_t irow = 0;
    Int_t ich = 0;

    for (int istrip = 0; istrip < nStrips; istrip++) {
      int ich = 24 * iSM + istrip;
      // LG
      hwAddress = GetHWAddressLEDRef(istrip, 0);
      DecodeHWAddress(hwAddress, branch, FEC, chip, chan);
      fMeanPed[iSM][iRCU][branch][FEC][chip][chan] =
          short(mHistoLEDLG[iSM]->GetBinContent(istrip + 1) + 0.5); // fix 0.5?
      // HG
      hwAddress = GetHWAddressLEDRef(istrip, 1);
      DecodeHWAddress(hwAddress, branch, FEC, chip, chan);
      fMeanPed[iSM][iRCU][branch][FEC][chip][chan] =
          short(mHistoLEDHG[iSM]->GetBinContent(istrip + 1) + 0.5); // fix 0.5?
    }
  }

  return;
}

Int_t GetHWAddress(Int_t iside, Int_t icol, Int_t irow, CHTYP igain,
                   Int_t &iRCU) {
  iRCU = -111;

  //  const auto& map = mMapper->getMappingForDDL(feeID);

  // RCU0
  if (0 <= irow && irow < 8)
    iRCU = 0; // first cable row
  else if (8 <= irow && irow < 16 && 0 <= icol && icol < 24)
    iRCU = 0; // first half;
  // second cable row
  // RCU1
  else if (8 <= irow && irow < 16 && 24 <= icol && icol < 48)
    iRCU = 1; // second half;
  // second cable row
  else if (16 <= irow && irow < 24)
    iRCU = 1; // third cable row

  // swap for odd=C side, to allow us to cable both sides the same
  Int_t iRCUSide = iRCU;
  if (iside == 1) {
    iRCU = 1 - iRCU;
    iRCUSide = iRCU + 2; // to make it map file index
  }

  // Int_t hwAddress = fMapping[iRCUSide]->GetHWAddress(irow, icol, igain);

  if (igain == CHTYP::LEDMON) {
    if (iside == 0)
      iRCUSide = 0;
    else
      iRCUSide = 2;
  }

  Int_t hwaddress =
      mMapper->getMappingForDDL(iRCUSide).getHardwareAddress(irow, icol, igain);

  return hwaddress;
}

Int_t GetHWAddressLEDRef(Int_t istrip, Int_t igain) {
  Int_t iRCU = 0;     // for both sides; LED ref info is the same for both sides
  Int_t caloflag = 3; // AliCaloRawStreamV3::kLEDMonData;

  Int_t hwAddress = fHWAddrLEDRef[istrip][igain];

  return hwAddress;
}

void FillTextFile(const char *filename) {
  const char *sideStr[] = {"A", "C"};
  const char *branchStr[] = {"A", "B"};
  int VFPED = 0x06;
  int SRUAltroWrite = 0x40000000;
  int Port = 0x1001; // 4097

  char cmd[500];

  //  char filename[100];
  //  sprintf(filename, "pedestalData_fromdata.txt");
  ofstream fout(filename);

  /*
    sprintf(cmd,"echo \"cp info.txt GeneratePedestalScriptSRU.C %s/.\" >
   scp.sh", dirname); gSystem->Exec(cmd); sprintf(cmd,"echo \"scp -r -p %s
   arc24:pedestals/.\" >> scp.sh", dirname); gSystem->Exec(cmd);
     sprintf(cmd,"echo \"ssh -t arc24 \'cd pedestals; scp -r -p %s
   emc@alidcscom702:EMCALControl_2_Martin/pedestals/.\'\" >> scp.sh", dirname);
   gSystem->Exec(cmd);
  */
  char scriptLine[200];
  unsigned int lineValue = 0;

  const unsigned int FECheaderCode = 0xC0000000;
  //  const unsigned int FECwordCode   = 0x80000000;
  const unsigned int FEClineCode = 0x40000000;

  Int_t iSM = 0;
  Int_t iRCU = 0;
  Int_t ibranch = 0;
  Int_t iFEC = 0;
  Int_t ichip = 0;
  Int_t ichan = 0;
  Int_t Ped = 0;
  Int_t iDTC = 0;

  for (iSM = 0; iSM < kNSM; iSM++) {
    int iside = iSM % 2;
    int isect = iSM / 2;
    if (iSM > 11)
      isect += 3; // skip non-installed sectors

    int activeDTC[kNDTC] = {0};
    for (iDTC = 0; iDTC < kNDTC; iDTC++) {
      if (iDTC == 10 || iDTC == 20 || iDTC == 30) { // skip TRU
        activeDTC[iDTC] = 0;
      } else {
        if (iSM < 10) { // not special third SMs or DCal SMs
          activeDTC[iDTC] = 1;
        } else {
          if (iSM == 10 || iSM == 19) { // SMA5 or SMC12
            if (iDTC < 14) {
              activeDTC[iDTC] = 1;
            } else {
              activeDTC[iDTC] = 0;
            }
          } else if (iSM == 11 || iSM == 18) { // SMC5 or SMA12
            if (iDTC == 0 || iDTC >= 27) {
              activeDTC[iDTC] = 1;
            } else {
              activeDTC[iDTC] = 0;
            }
          } else {
            // DCal... no FECs in  9,11-13, 23-26, 36-39
            if ((iDTC >= 9 && iDTC <= 13) || (iDTC >= 23 && iDTC <= 26) ||
                (iDTC >= 36 && iDTC <= 39)) {
              activeDTC[iDTC] = 0;
            } else {
              activeDTC[iDTC] = 1;
            }
          } // DCal
        }   // non-EMCal
      }     // non-TRU
    }

    // OK, let's generate the files for all active FECs/DTCs
    for (iDTC = 0; iDTC < kNDTC; iDTC++) {
      if (activeDTC[iDTC] == 0) {
        continue;
      }
      //      sprintf(filename, "%s/SM%1s%d/set_ped_DTC%02d.txt", dirname,
      //      sideStr[iside], isect, iDTC); ofstream fout(filename);

      lineValue = FECheaderCode | isect << 9 | iside << 8 | iDTC;
      //      sprintf(scriptLine, "0x%08x", lineValue);
      //      fout << scriptLine << endl;
      fout << lineValue << endl;

      iRCU = iDTC / 20;
      ibranch = (iDTC % 20) / 10;
      iFEC = iDTC % 10;
      int ipos = iFEC + 10 * ibranch;

      // write DTC file header..
      //      sprintf(scriptLine, "%s # IP\n%d       #UDP port", IP, Port);
      //      fout << scriptLine << endl;

      int dtcselUpper = 0;
      int dtcselLower = 0;
      if (iRCU == 0) {
        dtcselLower = (1 << ipos);
      } else { // crate == 1
        dtcselUpper = (1 << ipos);
      }

      //      lineValue = FECwordCode | dtcselUpper;
      //      fout << lineValue << endl;
      //      lineValue = FECwordCode | dtcselLower;
      //      fout << lineValue << endl;

      //      lineValue = FECwordCode | 0x3;
      //      fout << lineValue << endl;
      //      lineValue = FECwordCode | (iFEC + 16*ibranch);
      //      fout << lineValue << endl;

      for (ichip = 0; ichip < kNChip; ichip++) { // ALTRO 0,2,3,4
        if (ichip != 1) {
          for (ichan = 0; ichan < kNChan; ichan++) {

            if (iFEC != 0 || (ichan < 8 || ichan > 11)) {

              Ped =
                  TMath::Nint(fMeanPed[iSM][iRCU][ibranch][iFEC][ichip][ichan]);
              // raise Ped value to max for channels with exceptionally large
              // RMS
              if (fRmsPed[iSM][iRCU][ibranch][iFEC][ichip][ichan] > kBadRMS) {
                printf(" bad pedestal RMS: iSM %d iRCU %d ibranch %d iFEC %d "
                       "ichip %d ichan %d - raising from %d to 0x3ff\n",
                       iSM, iRCU, ibranch, iFEC, ichip, ichan, Ped);
                Ped = 0x3ff;
              }
              //
              //          int writeAddr = SRUAltroWrite | (ibranch << 16) |
              //          (iFEC << 12) | (ichip << 9) | (ichan << 5) | VFPED;
              int writeAddr = (ichip << 4) | ichan;
              //          sprintf(scriptLine, "%08x # Branch %s, Card %d, Altro
              //          %d, Chan %d", writeAddr, branchStr[ibranch], iFEC,
              //          ichip, ichan); fout << scriptLine << endl;

              //          int writeVal = Ped;
              //          sprintf(scriptLine, "%08x # Pedestal 0x%x = %d",
              //          writeVal, Ped, Ped); fout << scriptLine << endl;

              lineValue = FEClineCode | (writeAddr << 12) | Ped;
              //          sprintf(scriptLine, "0x%08x", lineValue);
              //          fout << scriptLine << endl;
              fout << lineValue << endl;
            }
          }
        }
      } // chip

    } // iDTC
  }   // iSM

  fout.close();

  return;
}
