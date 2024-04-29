/*
processing macro for pedestal data from ccdb


 return codes:
 0 - OK
 1 - input file not found
 2 - run number mismatch
 3 - number of lines
 4 - corrupted data
 5 - trailer word not found
 6 - couldn't create dir
 7 - wrong number of arguments

author: M.Poghosyan 26.04.2024
*/

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

const unsigned int FECheaderCode = 0xC0000000;
const unsigned int FECwordCode = 0x80000000;
const unsigned int FEClineCode = 0x40000000;
const unsigned int TRAILER = 0xFFFFFFFF;
const unsigned SRUAltroWrite = 0x40000000;
const char *sideStr[] = {"A", "C"};
const int Port = 0x1001; // 4097
const int VFPED = 0x06;
const int NLINES = 36862;

char baseDir[100];

int debug = 0;

bool CreateDir(std::string name) {

  char dirname[200];
  sprintf(dirname, "%s/%s", baseDir, name.data());

  if (mkdir(dirname, 0777) == -1) {
    std::cerr << "Error :  problem with creating the directory : " << dirname
              << std::endl;
    return false;
  } else {
    std::cout << dirname << " Directory created" << std::endl;
    return true;
  }

  return false;
}

void help() {
  std::cout << "********************************\n"
            << "analPedFile_EMC.c usage:\n\n"
            << "./analPedFile_EMC runNumber debug\n\n"
            << "debug = 0 by default\n"
            << "********************************\n";
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    help();
    return 7;
  }

  int runNumber1 = atoi(argv[1]);
  int lcounter = 0;

  if (argc > 2) {
    debug = atoi(argv[2]);
  }

  char buff[200];
  sprintf(buff, "file-%d", runNumber1);

  const std::string inputFile(buff);
  std::fstream ifs(inputFile.data(), std::ios_base::in);

  if (!ifs) {
    std::cout << "ERROR: input file not found: " << inputFile.data() << "\n";
    return 1;
  }

  int runNumber;
  ifs >> runNumber;
  lcounter++;

  if (runNumber != runNumber1) {
    std::cout << "ERROR: found runNumber = " << runNumber
              << "is different from the expeced one " << runNumber1 << "\n";
    return 2;
  }

  if (debug > 0)
    std::cout << "runNumber = " << runNumber << std::endl;

  sprintf(baseDir, "%s/%d", "../pedestals", runNumber);

  if (!CreateDir("")) {
    std::cout << "ERROR: Directory already exists??? \n";
    return 6;
  }

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
    lcounter++;

    if (debug > 1)
      std::cout << "val: " << val << std::endl;

    if (val == TRAILER) {
      if (lcounter == NLINES) {
        break;
      } else {
        std::cout << "ERROR: " << lcounter << " found instead of " << NLINES
                  << "\n";
        return 3;
      }
    }

    if ((val >> 30) == (FECheaderCode >> 30)) {

      int iSector_tmp = (val >> 9) & 0xFF;
      int iSide_tmp = (val >> 8) & 0x1;
      int iDTC_tmp = val & 0xFF;

      if ((iSector != iSector_tmp) || (iSide != iSide_tmp)) {
        // new SM

        iSector = iSector_tmp;
        iSide = iSide_tmp;

        sprintf(SMname, "SM%s%d", sideStr[iSide], iSector);

        if (debug > 0)
          std::cout << SMname << std::endl;

        CreateDir(SMname);
      }

      if (iDTC != iDTC_tmp) {
        // new DTC
        iDTC = iDTC_tmp;
        sprintf(filename, "%s/SM%1s%d/set_ped_DTC%02d.txt", baseDir,
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
          return 4;
        }

        sprintf(scriptLine, "%s # IP\n%d       #UDP port", IP, Port);
        fout << scriptLine << std::endl;
        sprintf(scriptLine, "%08x\n%08x", dtcselUpper, dtcselLower);
        fout << scriptLine << std::endl;
        sprintf(scriptLine, "%08x\n%08x", 0x3, (iFEC + 16 * ibranch));
        fout << scriptLine << std::endl;
      }
    }

    if ((val >> 30) == (FEClineCode >> 30)) {
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
  }

  if (val != TRAILER) {
    std::cout << "ERROR: trailer word not found\n";
    return 5;
  }

  if (fout.is_open())
    fout.close();

  ifs.close();

  return 0;
}
