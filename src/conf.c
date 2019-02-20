/**
 *  Copyright (c) 2017 Leonardo A. Bautista-Gomez
 *  All rights reserved
 *
 *  FTI - A multi-level checkpointing library for C/C++/Fortran applications
 *
 *  Revision 1.0 : Fault Tolerance Interface (FTI)
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  @file   conf.c
 *  @date   October, 2017
 *  @brief  Configuration loading functions for the FTI library.
 */

#include "interface.h"
#include "api_cuda.h"

/*-------------------------------------------------------------------------*/
/**
  @brief      Sets the exec. ID and failure parameters in the conf. file.
  @param      FTI_Conf        Configuration metadata.
  @param      FTI_Exec        Execution metadata.
  @param      restart         Value to set in the conf. file (0 or 1).
  @return     integer         FTI_SCES if successful.

  This function sets the execution ID and failure parameters in the
  configuration file. This is to avoid forcing the user to change these
  values manually in case of recovery needed. In this way, relaunching the
  execution in the same way as the initial time will make FTI detect that
  it is a restart. It also allows to set the failure parameter back to 0
  at the end of a successful execution.

 **/
/*-------------------------------------------------------------------------*/
int FTI_UpdateConf(FTIT_configuration* FTI_Conf, FTIT_execution* FTI_Exec, int restart)
{
    char str[FTI_BUFS]; //For console output

    // Load dictionary
    dictionary* ini = iniparser_load(FTI_Conf->cfgFile);
    snprintf(str, FTI_BUFS, "Updating configuration file (%s)...", FTI_Conf->cfgFile);
    FTI_Print(str, FTI_DBUG);
    if (ini == NULL) {
        FTI_Print("Iniparser failed to parse the conf. file.", FTI_WARN);
        return FTI_NSCS;
    }

    snprintf(str, FTI_BUFS, "%d", restart);
    // Set failure to 'restart'
    iniparser_set(ini, "Restart:failure", str);
    // Set the exec. ID
    iniparser_set(ini, "Restart:exec_id", FTI_Exec->id);

    FILE* fd = fopen(FTI_Conf->cfgFile, "w");
    if (fd == NULL) {
        FTI_Print("FTI failed to open the configuration file.", FTI_EROR);

        iniparser_freedict(ini);

        return FTI_NSCS;
    }

    // Write new configuration
    iniparser_dump_ini(ini, fd);

    if (fclose(fd) != 0) {
        FTI_Print("FTI failed to close the configuration file.", FTI_EROR);

        iniparser_freedict(ini);

        return FTI_NSCS;
    }

    // Free dictionary
    iniparser_freedict(ini);

    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It reads the configuration given in the configuration file.
  @param      FTI_Conf        Configuration metadata.
  @param      FTI_Exec        Execution metadata.
  @param      FTI_Topo        Topology metadata.
  @param      FTI_Ckpt        Checkpoint metadata.
  @param      FTI_Inje        Type to describe failure injections in FTI.
  @return     integer         FTI_SCES if successful.

  This function reads the configuration given in the FTI configuration
  file and sets other required parameters.

 **/
/*-------------------------------------------------------------------------*/
int FTI_ReadConf(FTIT_configuration* FTI_Conf, FTIT_execution* FTI_Exec,
        FTIT_topology* FTI_Topo, FTIT_checkpoint* FTI_Ckpt,
        FTIT_injection* FTI_Inje)
{
    char str[FTI_BUFS]; //For console output
    snprintf(str, FTI_BUFS, "Reading FTI configuration file (%s)...", FTI_Conf->cfgFile);
    FTI_Print(str, FTI_INFO);

    // Check access to FTI configuration file and load dictionary
    if (access(FTI_Conf->cfgFile, F_OK) != 0) {
        FTI_Print("FTI configuration file NOT accessible.", FTI_WARN);
        return FTI_NSCS;
    }
    dictionary* ini = iniparser_load(FTI_Conf->cfgFile);
    if (ini == NULL) {
        FTI_Print("Iniparser failed to parse the conf. file.", FTI_WARN);
        return FTI_NSCS;
    }

    // Setting/reading checkpoint configuration metadata
    char *par = iniparser_getstring(ini, "Basic:ckpt_dir", NULL);
    snprintf(FTI_Conf->localDir, FTI_BUFS, "%s", par);
    par = iniparser_getstring(ini, "Basic:glbl_dir", NULL);
    snprintf(FTI_Conf->glbalDir, FTI_BUFS, "%s", par);
    par = iniparser_getstring(ini, "Basic:meta_dir", NULL);
    snprintf(FTI_Conf->metadDir, FTI_BUFS, "%s", par);
    FTI_Ckpt[1].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l1", -1);
    FTI_Ckpt[2].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l2", -1);
    FTI_Ckpt[3].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l3", -1);
    FTI_Ckpt[4].ckptDcpIntv = (int)iniparser_getint(ini, "Basic:dcp_l4", 0); // 0 -> disabled
    FTI_Ckpt[4].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l4", -1);
    FTI_Ckpt[1].isInline = (int)1;
    FTI_Ckpt[2].isInline = (int)iniparser_getint(ini, "Basic:inline_l2", 1);
    FTI_Ckpt[3].isInline = (int)iniparser_getint(ini, "Basic:inline_l3", 1);
    FTI_Ckpt[4].isInline = (int)iniparser_getint(ini, "Basic:inline_l4", 1);
    FTI_Ckpt[1].ckptCnt  = 1;
    FTI_Ckpt[2].ckptCnt  = 1;
    FTI_Ckpt[3].ckptCnt  = 1;
    FTI_Ckpt[4].ckptCnt  = 1;
    FTI_Ckpt[4].ckptDcpCnt  = 1;

    FTI_Conf->stagingEnabled = (bool)iniparser_getboolean(ini, "Basic:enable_staging", 0);

    // Reading/setting configuration metadata
    FTI_Conf->keepHeadsAlive = (bool)iniparser_getboolean(ini, "Basic:keep_heads_alive", 0);
    FTI_Conf->dcpEnabled = (bool)iniparser_getboolean(ini, "Basic:enable_dcp", 0);
    FTI_Conf->dcpMode = (int)iniparser_getint(ini, "Basic:dcp_mode", -1) + FTI_DCP_MODE_OFFSET;
    FTI_Conf->dcpBlockSize = (int)iniparser_getint(ini, "Basic:dcp_block_size", -1);
    FTI_Conf->verbosity = (int)iniparser_getint(ini, "Basic:verbosity", -1);
    FTI_Conf->saveLastCkpt = (int)iniparser_getint(ini, "Basic:keep_last_ckpt", 0);
    FTI_Conf->keepL4Ckpt = (bool)iniparser_getboolean(ini, "Basic:keep_l4_ckpt", 0);
    FTI_Conf->blockSize = (int)iniparser_getint(ini, "Advanced:block_size", -1) * 1024;
    FTI_Conf->transferSize = (int)iniparser_getint(ini, "Advanced:transfer_size", -1) * 1024 * 1024;
    FTI_Conf->ckptTag = (int)iniparser_getint(ini, "Advanced:ckpt_tag", 711);
    FTI_Conf->stageTag = (int)iniparser_getint(ini, "Advanced:stage_tag", 406);
    FTI_Conf->finalTag = (int)iniparser_getint(ini, "Advanced:final_tag", 3107);
    FTI_Conf->generalTag = (int)iniparser_getint(ini, "Advanced:general_tag", 2612);
    FTI_Conf->test = (int)iniparser_getint(ini, "Advanced:local_test", -1);
    FTI_Conf->l3WordSize = FTI_WORD;
    FTI_Conf->ioMode = (int)iniparser_getint(ini, "Basic:ckpt_io", 0) + 1000;
#ifdef GPUSUPPORT
    FTI_Conf->cHostBufSize = (size_t)iniparser_getlint(ini, "Advanced:gpu_host_bufsize", FTI_DEFAULT_CHOSTBUF_SIZE_MB) * ((size_t)1 << 20);
#endif
#ifdef LUSTRE
    FTI_Conf->stripeUnit = (int)iniparser_getint(ini, "Advanced:lustre_stiping_unit", 4194304);
    FTI_Conf->stripeFactor = (int)iniparser_getint(ini, "Advanced:lustre_stiping_factor", -1);
    FTI_Conf->stripeOffset = (int)iniparser_getint(ini, "Advanced:lustre_stiping_offset", -1);
#endif
#ifdef _USE_AML
	FTI_Conf->numanodeFast = (int)iniparser_getint(ini, "Advanced:numanodefast", 1);
	FTI_Conf->numanodeSlow = (int)iniparser_getint(ini, "Advanced:numanodeslow", 0);
#endif 

    // Reading/setting execution metadata
    FTI_Exec->nbVar = 0;
    FTI_Exec->nbType = 0;
    FTI_Exec->ckpt = 0;
    FTI_Exec->minuteCnt = 0;
    FTI_Exec->ckptCnt = 1;
    FTI_Exec->ckptIcnt = 0;
    FTI_Exec->ckptID = 0;
    FTI_Exec->ckptLvel = 0;
    FTI_Exec->ckptIntv = 1;
    FTI_Exec->wasLastOffline = 0;
    FTI_Exec->ckptNext = 0;
    FTI_Exec->ckptLast = 0;
    FTI_Exec->syncIter = 1;
    FTI_Exec->syncIterMax = (int)iniparser_getint(ini, "Basic:max_sync_intv", -1);
    FTI_Exec->lastIterTime = 0;
    FTI_Exec->totalIterTime = 0;
    FTI_Exec->meanIterTime = 0;
    FTI_Exec->metaAlloc = 0;
    FTI_Exec->reco = (int)iniparser_getint(ini, "restart:failure", 0);
    if (FTI_Exec->reco == 0) {
        time_t tim = time(NULL);
        struct tm* n = localtime(&tim);
        snprintf(FTI_Exec->id, FTI_BUFS, "%d-%02d-%02d_%02d-%02d-%02d",
                n->tm_year + 1900, n->tm_mon + 1, n->tm_mday, n->tm_hour, n->tm_min, n->tm_sec);
        MPI_Bcast(FTI_Exec->id, FTI_BUFS, MPI_CHAR, 0, FTI_Exec->globalComm);
        snprintf(str, FTI_BUFS, "The execution ID is: %s", FTI_Exec->id);
        FTI_Print(str, FTI_INFO);
    }
    else {
        par = iniparser_getstring(ini, "restart:exec_id", NULL);
        snprintf(FTI_Exec->id, FTI_BUFS, "%s", par);
        snprintf(str, FTI_BUFS, "This is a restart. The execution ID is: %s", FTI_Exec->id);
        FTI_Print(str, FTI_INFO);
    }

    // Reading/setting topology metadata
    FTI_Topo->nbHeads = (int)iniparser_getint(ini, "Basic:head", 0);
    FTI_Topo->groupSize = (int)iniparser_getint(ini, "Basic:group_size", -1);
    FTI_Topo->nodeSize = (int)iniparser_getint(ini, "Basic:node_size", -1);
    FTI_Topo->nbApprocs = FTI_Topo->nodeSize - FTI_Topo->nbHeads;
    FTI_Topo->nbNodes = (FTI_Topo->nodeSize) ? FTI_Topo->nbProc / FTI_Topo->nodeSize : 0;

    // Reading/setting injection parameters
    FTI_Inje->rank = (int)iniparser_getint(ini, "Injection:rank", 0);
    FTI_Inje->index = (int)iniparser_getint(ini, "Injection:index", 0);
    FTI_Inje->position = (int)iniparser_getint(ini, "Injection:position", 0);
    FTI_Inje->number = (int)iniparser_getint(ini, "Injection:number", 0);
    FTI_Inje->frequency = (int)iniparser_getint(ini, "Injection:frequency", -1);

    // Synchronize after config reading and free dictionary
    MPI_Barrier(FTI_Exec->globalComm);

    iniparser_freedict(ini);

    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It tests that the configuration given is correct.
  @param      FTI_Conf        Configuration metadata.
  @param      FTI_Topo        Topology metadata.
  @param      FTI_Ckpt        Checkpoint metadata.
  @param      FTI_Exec        Execution metadata.
  @return     integer         FTI_SCES if successful.

  This function tests the FTI configuration to make sure that all
  parameter's values are correct.

 **/
/*-------------------------------------------------------------------------*/
int FTI_TestConfig(FTIT_configuration* FTI_Conf, FTIT_topology* FTI_Topo,
        FTIT_checkpoint* FTI_Ckpt, FTIT_execution* FTI_Exec)
{
	char errString[512];
    // Check requirements.
    if (FTI_Topo->nbHeads != 0 && FTI_Topo->nbHeads != 1) {
        FTI_Print("The number of heads needs to be set to 0 or 1.", FTI_WARN);
        return FTI_NSCS;
    }
    if (FTI_Topo->nbProc % FTI_Topo->nodeSize != 0) {
		snprintf(errString, 512, "Number of ranks (%d) is not a multiple of the node size (%d)",
		FTI_Topo->nbProc , FTI_Topo->nodeSize);
        FTI_Print(errString, FTI_WARN);
     //   return FTI_NSCS;
    }
    if (FTI_Topo->nbNodes % FTI_Topo->groupSize != 0) {
        snprintf(errString, 512, "The number of nodes (%d) is not multiple of the group size (%d)." ,
		FTI_Topo->nbNodes , FTI_Topo->groupSize );
        FTI_Print(errString, FTI_WARN);
        return FTI_NSCS;
    }
    // Check if Reed-Salomon and L2 checkpointing is requested.
    int L2req = (FTI_Ckpt[2].ckptIntv > 0) ? 1 : 0;
    int RSreq = (FTI_Ckpt[3].ckptIntv > 0) ? 1 : 0;
    if (FTI_Topo->groupSize <= 2 && (L2req || RSreq)) {
	snprintf(errString, 512, "The group size (%d) must be bigger than 2", FTI_Topo->groupSize  );
		FTI_Print(errString, FTI_WARN);
        return FTI_NSCS;
    }
    if (FTI_Topo->groupSize >= 32 && RSreq) {
	snprintf(errString, 512,  "The group size (%d) must be lower than 32",  FTI_Topo->groupSize );
		 FTI_Print(errString, FTI_WARN);
        return FTI_NSCS;
    }
    if (FTI_Conf->verbosity > 3 || FTI_Conf->verbosity < 1) {
        FTI_Print("Verbosity needs to be set to 1, 2 or 3.", FTI_WARN);
		 FTI_Print(errString, FTI_WARN);
        return FTI_NSCS;
    }
    if (FTI_Conf->blockSize > (2048 * 1024) || FTI_Conf->blockSize < (1 * 1024)) {
        FTI_Print("Block size needs to be set between 1 and 2048.", FTI_WARN);
			 FTI_Print(errString, FTI_WARN);
        return FTI_NSCS;
    }

    if ( FTI_Conf->keepHeadsAlive && ( FTI_Topo->nbHeads == 0 ) ) {
        FTI_Print("Head feature is disabled but 'keep_heads_alive' is activated. Incompatiple setting!.", FTI_WARN);
        return FTI_NSCS;
    }

    // check dCP settings only if dCP is enabled
    if ( FTI_Conf->dcpEnabled ) {
        if ( !(FTI_Conf->ioMode == FTI_IO_FTIFF) ) {
            FTI_Print("dCP may only be used with FTI-FF enabled, dCP disabled.", FTI_WARN);
            FTI_Conf->dcpEnabled = false;
            goto CHECK_DCP_SETTING_END;
        }
        if ( (FTI_Conf->dcpMode < FTI_DCP_MODE_MD5) || (FTI_Conf->dcpMode > FTI_DCP_MODE_CRC32) ) {
            FTI_Print("dCP mode ('Basic:dcp_mode') must be either 1 (MD5) or 2 (CRC32), dCP disabled.", FTI_WARN);
            FTI_Conf->dcpEnabled = false;
            goto CHECK_DCP_SETTING_END;
        }
        if ( (FTI_Conf->dcpBlockSize < 512) || (FTI_Conf->dcpBlockSize > USHRT_MAX) ) {
            char str[FTI_BUFS];
            snprintf( str, FTI_BUFS, "dCP block size ('Basic:dcp_block_size') must be between 512 and %d bytes, dCP disabled", USHRT_MAX );
            FTI_Print( str, FTI_WARN );
            FTI_Conf->dcpEnabled = false;
            goto CHECK_DCP_SETTING_END;
        }
        if (FTI_Ckpt[4].ckptDcpIntv > 0 && !(FTI_Conf->dcpEnabled)) {
            FTI_Print( "L4 dCP interval set, but, dCP is disabled! Setting will be ignored.", FTI_WARN );
            FTI_Ckpt[4].ckptDcpIntv = 0;
            FTI_Conf->dcpEnabled = false;
            goto CHECK_DCP_SETTING_END;
        }
    }

CHECK_DCP_SETTING_END:
    
    if (FTI_Conf->transferSize > (1024 * 1024 * 64) || FTI_Conf->transferSize < (1024 * 1024 * 8)) {
        FTI_Print("Transfer size (default = 16MB) not set in Cofiguration file.", FTI_WARN);
        FTI_Conf->transferSize = 16 * 1024 * 1024;
    }
    if (FTI_Conf->test != 0 && FTI_Conf->test != 1) {
        FTI_Print("Local test size needs to be set to 0 or 1.", FTI_WARN);
        return FTI_NSCS;
    }
    if (FTI_Conf->saveLastCkpt != 0 && FTI_Conf->saveLastCkpt != 1) {
        FTI_Print("Keep last ckpt. needs to be set to 0 or 1.", FTI_WARN);
        return FTI_NSCS;
    }
    int i;
    for (i = 1; i < 5; i++) {
        if (FTI_Ckpt[i].ckptIntv == 0) {
            FTI_Ckpt[i].ckptIntv = -1;
        }
        if (FTI_Ckpt[i].isInline != 0 && FTI_Ckpt[i].isInline != 1) {
            FTI_Ckpt[i].isInline = 1;
        }
        if (FTI_Ckpt[i].isInline == 0 && FTI_Topo->nbHeads != 1) {
            FTI_Print("If inline is set to 0 then head should be set to 1.", FTI_WARN);
            return FTI_NSCS;
        }
    }
    if (FTI_Exec->syncIterMax < 0) {
        FTI_Exec->syncIterMax = 512;
        FTI_Print("Variable 'Basic:max_sync_intv' is not set. Set to default (512 iterations).", FTI_WARN);
    } else if ((FTI_Exec->syncIterMax & (FTI_Exec->syncIterMax - 1)) != 0) {
        int check = 1;
        while (((check << 1 ) < FTI_Exec->syncIterMax) && ((check << 1) > 0)) {
            check = check << 1;
        }
        FTI_Exec->syncIterMax = check;
        char str[FTI_BUFS];
        snprintf(str, FTI_BUFS, "Maximal sync. intv. has to be a power of 2. Set to nearest lower value (%d iterations)", FTI_Exec->syncIterMax);
        FTI_Print(str, FTI_WARN);
    } else if (FTI_Exec->syncIterMax == 0) {
        FTI_Exec->syncIterMax = 512;
        FTI_Print("Variable 'Basic:max_sync_intv' is set to default (512 iterations).", FTI_DBUG);
    }
    if ( FTI_Conf->stagingEnabled && !FTI_Topo->nbHeads ) {
        FTI_Print( "Staging is enabled but no dedicated head process, staging will be performed inline!", FTI_WARN );
    }
    if (FTI_Topo->groupSize < 1) {
        FTI_Topo->groupSize = 1;
    }
    switch (FTI_Conf->ioMode) {
        case FTI_IO_POSIX:
            FTI_Print("Selected Ckpt I/O is POSIX", FTI_INFO);
            break;
        case FTI_IO_MPI:
            FTI_Print("Selected Ckpt I/O is MPI-I/O", FTI_INFO);
            break;
        case FTI_IO_FTIFF:
            FTI_Print("Selected Ckpt I/O is FTI-FF", FTI_INFO);
            break;
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
            case FTI_IO_SIONLIB:
                FTI_Print("Selected Ckpt I/O is SIONLIB", FTI_INFO);
                break;
#endif
        case FTI_IO_HDF5:
#ifdef ENABLE_HDF5 // --> If HDF5 is installed
                FTI_Print("Selected Ckpt I/O is HDF5", FTI_INFO);
#else
                FTI_Print("Selected Ckpt I/O is HDF5, but HDF5 is not enabled. Setting IO mode to POSIX.", FTI_WARN);
                FTI_Conf->ioMode = FTI_IO_POSIX;
#endif
            break;
        default:
            FTI_Conf->ioMode = FTI_IO_POSIX;
            FTI_Print("Variable 'Basic:ckpt_io' is not set. Set to default (POSIX).", FTI_WARN);
            break;

    }
        return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It tests that the directories given is correct.
  @param      FTI_Conf        Configuration metadata.
  @param      FTI_Topo        Topology metadata.
  @return     integer         FTI_SCES if successful.

  This function tests that the directories given in the FTI configuration
  are correct.

 **/
/*-------------------------------------------------------------------------*/
int FTI_TestDirectories(FTIT_configuration* FTI_Conf, FTIT_topology* FTI_Topo)
{
    char str[FTI_BUFS]; //For console output

    // Checking local directory
    snprintf(str, FTI_BUFS, "Checking the local directory (%s)...", FTI_Conf->localDir);
    FTI_Print(str, FTI_DBUG);
    if (mkdir(FTI_Conf->localDir, 0777) == -1) {
        if (errno != EEXIST) {
            FTI_Print("The local directory could NOT be created.", FTI_WARN);
            return FTI_NSCS;
        }
    }

    if (FTI_Topo->myRank == 0) {
        // Checking metadata directory
        snprintf(str, FTI_BUFS, "Checking the metadata directory (%s)...", FTI_Conf->metadDir);
        FTI_Print(str, FTI_DBUG);
        if (mkdir(FTI_Conf->metadDir, 0777) == -1) {
            if (errno != EEXIST) {
                FTI_Print("The metadata directory could NOT be created.", FTI_WARN);
                return FTI_NSCS;
            }
        }

        // Checking global directory
        snprintf(str,FTI_BUFS,  "Checking the global directory (%s)...", FTI_Conf->glbalDir);
        FTI_Print(str, FTI_DBUG);
        if (mkdir(FTI_Conf->glbalDir, 0777) == -1) {
            if (errno != EEXIST) {
                FTI_Print("The global directory could NOT be created.", FTI_WARN);
                return FTI_NSCS;
            }
        }
    }
    //Waiting for metadDir being created
    MPI_Barrier(FTI_COMM_WORLD);

    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It creates the directories required for current execution.
  @param      FTI_Conf        Configuration metadata.
  @param      FTI_Exec        Execution metadata.
  @param      FTI_Topo        Topology metadata.
  @param      FTI_Ckpt        Checkpoint metadata.
  @return     integer         FTI_SCES if successful.

  This function creates the temporary metadata, local and global
  directories required for the current execution.

 **/
/*-------------------------------------------------------------------------*/
int FTI_CreateDirs(FTIT_configuration* FTI_Conf, FTIT_execution* FTI_Exec,
        FTIT_topology* FTI_Topo, FTIT_checkpoint* FTI_Ckpt)
{
    char strerr[FTI_BUFS];
    char fn[FTI_BUFS]; //Path of metadata directory

    // Create metadata timestamp directory
    snprintf(fn, FTI_BUFS, "%s/%s", FTI_Conf->metadDir, FTI_Exec->id);
    if (mkdir(fn, 0777) == -1) {
        if (errno != EEXIST) {
            FTI_Print("Cannot create metadata timestamp directory", FTI_EROR);
        }
    }
    snprintf(FTI_Conf->metadDir, FTI_BUFS, "%s", fn);
    snprintf(FTI_Conf->mTmpDir, FTI_BUFS, "%s/tmp", fn);
    snprintf(FTI_Ckpt[1].metaDir, FTI_BUFS, "%s/l1", fn);
    snprintf(FTI_Ckpt[2].metaDir, FTI_BUFS, "%s/l2", fn);
    snprintf(FTI_Ckpt[3].metaDir, FTI_BUFS, "%s/l3", fn);
    snprintf(FTI_Ckpt[4].metaDir, FTI_BUFS, "%s/l4", fn);

    // Create global checkpoint timestamp directory
    snprintf(fn, FTI_BUFS, "%s", FTI_Conf->glbalDir);
    snprintf(FTI_Conf->glbalDir, FTI_BUFS, "%s/%s", fn, FTI_Exec->id);
    if (mkdir(FTI_Conf->glbalDir, 0777) == -1) {
        if (errno != EEXIST) {
            FTI_Print("Cannot create global checkpoint timestamp directory", FTI_EROR);
        }
    }
    snprintf(FTI_Conf->gTmpDir, FTI_BUFS, "%s/tmp", FTI_Conf->glbalDir);
    snprintf(FTI_Ckpt[4].dcpDir, FTI_BUFS, "%s/dCP", FTI_Conf->glbalDir);
    snprintf(FTI_Ckpt[4].dcpName, FTI_BUFS, "dCPFile-Rank%d.fti", FTI_Topo->myRank);
    snprintf(FTI_Ckpt[4].dir, FTI_BUFS, "%s/l4", FTI_Conf->glbalDir);
    snprintf(FTI_Ckpt[4].archDir, FTI_BUFS, "%s/l4_archive", FTI_Conf->glbalDir);
    if ( FTI_Conf->keepL4Ckpt ) {
        if (mkdir(FTI_Ckpt[4].archDir, (mode_t) 0777) == -1) {
            if (errno != EEXIST) {
                snprintf(strerr, FTI_BUFS, "failed to create directory '%s', cannot keep L4 checkpoint.", FTI_Ckpt[4].archDir);
                FTI_Print(strerr, FTI_EROR);
                FTI_Conf->keepL4Ckpt = false;
            }
        }
    }

    // Create local checkpoint timestamp directory
    if (FTI_Conf->test) { // If local test generate name by topology
        snprintf(fn, FTI_BUFS, "%s/node%d", FTI_Conf->localDir, FTI_Topo->myRank / FTI_Topo->nodeSize);
        if (mkdir(fn, 0777) == -1) {
            if (errno != EEXIST) {
                FTI_Print("Cannot create local checkpoint timestamp directory", FTI_EROR);
            }
        }
    }
    else {
        snprintf(fn, FTI_BUFS, "%s", FTI_Conf->localDir);
    }
    snprintf(FTI_Conf->localDir, FTI_BUFS, "%s/%s", fn, FTI_Exec->id);
    if (mkdir(FTI_Conf->localDir, 0777) == -1) {
        if (errno != EEXIST) {
            FTI_Print("Cannot create local checkpoint timestamp directory", FTI_EROR);
        }
    }
    snprintf(FTI_Conf->lTmpDir, FTI_BUFS, "%s/tmp", FTI_Conf->localDir);
    snprintf(FTI_Ckpt[1].dir, FTI_BUFS, "%s/l1", FTI_Conf->localDir);
    snprintf(FTI_Ckpt[1].dcpDir, FTI_BUFS, "%s/dCP", FTI_Conf->localDir);
    snprintf(FTI_Ckpt[2].dir, FTI_BUFS, "%s/l2", FTI_Conf->localDir);
    snprintf(FTI_Ckpt[3].dir, FTI_BUFS, "%s/l3", FTI_Conf->localDir);
    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It reads and tests the configuration given.
  @param      FTI_Conf        Configuration metadata.
  @param      FTI_Exec        Execution metadata.
  @param      FTI_Topo        Topology metadata.
  @param      FTI_Ckpt        Checkpoint metadata.
  @param      FTI_Inje        Type to describe failure injections in FTI.
  @return     integer         FTI_SCES if successful.

  This function reads the configuration file. Then test that the
  configuration parameters are correct (including directories).

 **/
/*-------------------------------------------------------------------------*/
int FTI_LoadConf(FTIT_configuration* FTI_Conf, FTIT_execution* FTI_Exec,
        FTIT_topology* FTI_Topo, FTIT_checkpoint* FTI_Ckpt,
        FTIT_injection *FTI_Inje)
{
    int res = FTI_Try(FTI_ReadConf(FTI_Conf, FTI_Exec, FTI_Topo, FTI_Ckpt, FTI_Inje), "read configuration.");
    if (res == FTI_NSCS) {
        return FTI_NSCS;
    }
    res = FTI_Try(FTI_TestConfig(FTI_Conf, FTI_Topo, FTI_Ckpt, FTI_Exec), "pass the configuration test.");
    if (res == FTI_NSCS) {
        return FTI_NSCS;
    }
    res = FTI_Try(FTI_TestDirectories(FTI_Conf, FTI_Topo), "pass the directories test.");
    if (res == FTI_NSCS) {
        return FTI_NSCS;
    }
    res = FTI_Try(FTI_CreateDirs(FTI_Conf, FTI_Exec, FTI_Topo, FTI_Ckpt), "create checkpoint directories.");
    if (res == FTI_NSCS) {
        return FTI_NSCS;
    }
    return FTI_SCES;
}
