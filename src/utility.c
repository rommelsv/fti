#include <string.h>

#include "interface.h"
#include "ftiff.h"
#include "api_cuda.h"
#include "utility.h"


int write_posix(void *src, size_t size, void *opaque)
{
  FILE *fd = (FILE *)opaque;
  size_t written = 0;
  int fwrite_errno;
  char str[FTI_BUFS];

  while (written < size && !ferror(fd)) {
    errno = 0;
    written += fwrite(((char *)src) + written, 1, size - written, fd);
    fwrite_errno = errno;
  }

  if (ferror(fd)){
    char error_msg[FTI_BUFS];
    error_msg[0] = 0;
    strerror_r(fwrite_errno, error_msg, FTI_BUFS);
    snprintf(str, FTI_BUFS, "utility:c: (write_posix) Dataset could not be written: %s.", error_msg);
    FTI_Print(str, FTI_EROR);
    fclose(fd);
    return FTI_NSCS;
  }
  else
    return FTI_SCES;
}


int write_mpi(void *src, size_t size, void *opaque)
{
  WriteMPIInfo_t *write_info = (WriteMPIInfo_t *)opaque;
  size_t pos = 0;
  size_t bSize = write_info->FTI_Conf->transferSize;
  while (pos < size) {
    if ((size - pos) < write_info->FTI_Conf->transferSize) {
      bSize = size - pos;
    }

    MPI_Datatype dType;
    MPI_Type_contiguous(bSize, MPI_BYTE, &dType);
    MPI_Type_commit(&dType);

    write_info->err = MPI_File_write_at(write_info->pfh, write_info->offset, src, 1, dType, MPI_STATUS_IGNORE);
    // check if successful
    if (write_info->err != 0) {
      errno = 0;
      return FTI_NSCS;
    }
    MPI_Type_free(&dType);
    src += bSize;
    write_info->offset += bSize;
    pos = pos + bSize;
  }
  return FTI_SCES;
}



#ifdef ENABLE_SIONLIB 
int write_sion(void *src, size_t size, void *opaque)
{
  int *sid= (int *)opaque;
  int res = sion_fwrite(src, size, 1, *sid);
  if (res < 0 ){
    return FTI_NSCS;
  }
  return FTI_SCES;
}
#endif


int copyDataFromDevive(FTIT_execution* FTI_Exec, FTIT_dataset* FTI_Data){
#ifdef GPUSUPPORT
  int i;
  char str[FTI_BUFS]; //For console output
  for (i = 0; i < FTI_Exec->nbVar; i++) {
    if ( FTI_Data[i].isDevicePtr ){
      FTI_copy_from_device( FTI_Data[i].ptr, FTI_Data[i].devicePtr,FTI_Data[i].size,FTI_Exec);
    }
  }
#endif
  return FTI_SCES;
}

/* TODO: This is only a prototype, where the only "chunk" is the pageSize
   Ideally, we can use FTI_Data->dimention, to get the correct tiling scheme
   ether on 1D or 2D
*/
int write_posix_aligned(FTIT_execution * FTI_Exec, FTIT_dataset* FTI_Data, int fd){
#ifdef _USE_AML
    AML_DMA_LINUX_SEQ_DECL(dma);
    AML_TILING_1D_DECL(tiling);
    
    struct aml_area_linux_mmap_data mmapconfig;
    void * ptr; 
    int writtenBytes, i;
    
    aml_tiling_init(&tiling, AML_TILING_TYPE_1D, 
                        FTI_Exec->pageSize, FTI_Exec->pageSize );
    
    aml_area_linux_mmap_fd_init(&mmapconfig, fd, 0) ;
    ptr = aml_area_linux_mmap_generic(&mmapconfig, NULL, FTI_Exec->ckptSize);
    
    
    for( writtenBytes = 0, i = 0; i < FTI_Exec->nbVar; i++){
        void * ptrPosition = ptr + writtenBytes;
        int nrequests = FTI_Data[i].size / FTI_Exec->pageSize;

        aml_dma_linux_seq_init(&dma, nrequests);
    
        aml_dma_copy(&dma, 
                        &tiling, ptrPosition, 0,  
                        &tiling, FTI_Data[i].ptr, FTI_Data[i].size);

        writtenBytes +=  FTI_Data[i].size;
        aml_dma_linux_seq_destroy(&dma);
    }
    
    
    aml_tiling_destroy(&tiling, AML_TILING_TYPE_1D);
    munmap(ptr, FTI_Exec->ckptSize);

#endif

  return FTI_SCES;
}


