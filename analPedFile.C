// ifstream constructor.
#include <fstream>  // std::ifstream
#include <iostream> // std::cout
//#include <fstream>      // std::ofstream
#include <string>
#include<cstdlib>
#include <stdexcept>

//#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

const unsigned int FECheaderCode = 0xC0000000;
const unsigned int FECwordCode = 0x80000000;
const unsigned int FEClineCode = 0x40000000;
const char *sideStr[] = {"A", "C"};
const int Port = 0x1001; // 4097
const int VFPED = 0x06;
const int SRUAltroWrite = 0x40000000;

std::string baseDir = "pedestals_fromdata";

// void CreateDir(std::string name);
int debug = 0;//2;//0;

bool CreateDir(std::string name) {

  char dirname[200];
  sprintf(dirname, "%s/%s", baseDir.data(), name.data());

  if (mkdir(dirname, 0777) == -1) {
    //        std::cerr << "Error :  " << strerror(errno) << std::endl;
    std::cerr << "Error :  "
              << "problem with creating the directory" << std::endl;
    return false;
  } else {
    std::cout << dirname << " Directory created" << std::endl;
    return true;
  }

  return false;
}

void help() {
  std::cout << "********************************\n"
            << "analPedFile.c usage:\n\n"
            << "./a.out runNumber iDet iDebug\n\n"
            << "iDet = 0 for EMCAL, and iDet=1 for DCAL\n"
            << "iDebug = 0 by default\n"
            << "********************************\n";
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    help();
    return 1;
  }

  std::string runNumber(argv[1]);
  int iDET = atoi(argv[2]);

  if (argc >= 4) {
    debug = atoi(argv[3]);
  }

  std::cout << "runNumber = " << runNumber.data() << " for  "
            << ((iDET == 0) ? "EMCAL" : "DCAL") << std::endl;

  //  char dirname[200];
  //  sprintf(baseDir, "%s_%s", runNumber.data(), ((iDET == 0) ? "EMCAL" :
  //  "DCAL"));
  baseDir = "pedestalsRun3/" + runNumber;
  //     "pedestalsRun3/" + runNumber + "_" + ((iDET == 0) ? "EMCAL" : "DCAL");
  if (!CreateDir("")) {
    std::cout << "Directory already exists??? \n";
    //    return 1;
  }

  std::string inputFile = "pedestalData_" + runNumber + "_EMCAL.txt";
  if (iDET == 1) {
    inputFile = "pedestalData_" + runNumber + "_DCAL.txt";
  }

  std::fstream ifs(inputFile.data(), std::ios_base::in);

  if (!ifs) {
    throw std::runtime_error("input txt file not found");
  }

  //  char c = ifs.get();

  int iSector = -1;
  int iSide = -1;
  int iDTC = -1;
  int iRCU = -1;
  int ibranch = -1;
  int iFEC = -1;

  unsigned int val;
  char SMname[100];
  char filename[100];
  std::ofstream fout;

  char scriptLine[200];

  while (ifs >> val) {

    if ((val >> 30) == (FECheaderCode >> 30)) {

      int iSector_tmp = (val >> 9) & 0xFF;
      int iSide_tmp = (val >> 8) & 0x1;
      int iDTC_tmp = val & 0xFF;

      if (iDET == 0) {
        if (iSector_tmp > 5)
          continue;
      } else {
        if (iSector_tmp < 9)
          continue;
      }

      if ((iSector != iSector_tmp) || (iSide != iSide_tmp)) {
        // new SM

        iSector = iSector_tmp;
        iSide = iSide_tmp;

        sprintf(SMname, "SM%s%d", sideStr[iSide], iSector);
        CreateDir(SMname);
      }

      if (iDTC != iDTC_tmp) {
        // new DTC
        iDTC = iDTC_tmp;
        sprintf(filename, "%s/SM%1s%d/set_ped_DTC%02d.txt", baseDir.data(),
                sideStr[iSide], iSector, iDTC);

        if (debug > 0)
          std::cout << filename << std::endl;

        iRCU = iDTC / 20;
        ibranch = (iDTC % 20) / 10;
        iFEC = iDTC % 10;

        int ipos = iFEC + 10 * ibranch;

        int dtcselUpper = 0;
        int dtcselLower = 0;
        if (iRCU == 0) {
          dtcselLower = (1 << ipos);
        } else { // crate == 1
          dtcselUpper = (1 << ipos);
        }

        if (fout.is_open())
          fout.close();
        fout.open(filename, std::ofstream::out | std::ofstream::app);

        char IP[100];
        if (iSector == 0 && iSide == 0) {
          sprintf(IP, "10.160.36.158");
        } // SMA0
        else if (iSector == 0 && iSide == 1) {
          sprintf(IP, "10.160.36.159");
        } // SMC0
        else if (iSector == 1 && iSide == 0) {
          sprintf(IP, "10.160.36.160");
        } // SMA1
        else if (iSector == 1 && iSide == 1) {
          sprintf(IP, "10.160.36.161");
        } // SMC1
        //    else if (iSM == 4) { sprintf(IP, "10.160.36.162"); } // SMA2
        else if (iSector == 2 && iSide == 0) {
          sprintf(IP, "10.160.36.179");
        } // SMA2
        else if (iSector == 2 && iSide == 1) {
          sprintf(IP, "10.160.36.163");
        } // SMC2
        else if (iSector == 3 && iSide == 0) {
          sprintf(IP, "10.160.36.164");
        } // SMA3
        else if (iSector == 3 && iSide == 1) {
          sprintf(IP, "10.160.36.165");
        } // SMC3
        else if (iSector == 4 && iSide == 0) {
          sprintf(IP, "10.160.36.166");
        } // SMA4
        else if (iSector == 4 && iSide == 1) {
          sprintf(IP, "10.160.36.167");
        } // SMC4
        else if (iSector == 5 && iSide == 0) {
          sprintf(IP, "10.160.36.168");
        } // SMA5
        else if (iSector == 5 && iSide == 1) {
          sprintf(IP, "10.160.36.169");
        } // SMC5
        else if (iSector == 9 && iSide == 0) {
          sprintf(IP, "10.160.36.170");
        } // SMA9
        else if (iSector == 9 && iSide == 1) {
          sprintf(IP, "10.160.36.171");
        } // SMC9
        else if (iSector == 10 && iSide == 0) {
          sprintf(IP, "10.160.36.172");
        } // SMA10
        else if (iSector == 10 && iSide == 1) {
          sprintf(IP, "10.160.36.173");
        } // SMC10
        else if (iSector == 11 && iSide == 0) {
          sprintf(IP, "10.160.36.174");
        } // SMA11
        else if (iSector == 11 && iSide == 1) {
          sprintf(IP, "10.160.36.175");
        } // SMC11
        else if (iSector == 12 && iSide == 0) {
          sprintf(IP, "10.160.36.176");
        } // SMA12
        else if (iSector == 12 && iSide == 1) {
          sprintf(IP, "10.160.36.177");
        } // SMC12
        else {
          std::cout << "ERROR : iSector=" << iSector << " iSide = " << iSide
                    << std::endl;
          return 1;
        }

        sprintf(scriptLine, "%s # IP\n%d       #UDP port", IP, Port);
        fout << scriptLine << std::endl;
        sprintf(scriptLine, "%08x\n%08x", dtcselUpper, dtcselLower);
        fout << scriptLine << std::endl;
        sprintf(scriptLine, "%08x\n%08x", 0x3, (iFEC + 16 * ibranch));
        fout << scriptLine << std::endl;
      }
    }

    // else if( (val >> 30 ) == (FECwordCode >> 30) )
    //{
    //	sprintf(scriptLine, "%08x ", (val & 0x3FFFFFFF));
    //	fout << scriptLine << std::endl;
    //	std::cout << scriptLine << std::endl;
    //}

    if ((val >> 30) == (FEClineCode >> 30)) {
      //	std::cout << "line   : ";

      int ped = val & 0xFFF;
      int addr = (val & 0x3FFFFFFF) >> 12;
      int writeAddr =
          SRUAltroWrite | (ibranch << 16) | (iFEC << 12) | (addr << 5) | VFPED;

      if (debug > 1) {
        std::cout << SMname << " ibranch = " << ibranch << " FEC=" << iFEC
                  << std::endl;
      }

      sprintf(scriptLine, "%08x\n%08x", writeAddr, ped);
      fout << scriptLine << std::endl;
    }

    //	std::cout << " SM"<< sideStr[iSide] << iSector << "/DTC" << iDTC << " ";
    //    std::cout << val << std::endl;
  }

  if (fout.is_open())
    fout.close();

  ifs.close();

  return 0;
}
