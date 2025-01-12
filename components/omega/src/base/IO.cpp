//===-- base/IO.cpp - basic IO utilities implementation ---------*- C++ -*-===//
//
// The routines here provide the interfaces to the parallel IO environment,
// currently the SCORPIO parallel IO library.
// These functions should generally be accessed through the IOStreams
// interfaces and not used directly.
//
//===----------------------------------------------------------------------===//

#include "IO.h"
#include "DataTypes.h"
#include "Logging.h"
// include "Config.h"
#include "mpi.h"
#include "pio.h"

#include <map>
#include <string>

namespace OMEGA {
namespace IO {

// Define global variables
//------------------------------------------------------------------------------
int SysID               = 0;
FileFmt DefaultFileFmt  = FmtDefault;
Rearranger DefaultRearr = RearrDefault;

// Utilities
//------------------------------------------------------------------------------
// Converts string choice for PIO rearranger to an enum
Rearranger
RearrFromString(const std::string &Rearr // [in] choice of IO rearranger
) {
   // Set default return value
   Rearranger ReturnRearr = RearrUnknown;

   // Convert input string to lowercase for easier comparison
   std::string RearrComp = Rearr;
   std::transform(RearrComp.begin(), RearrComp.end(), RearrComp.begin(),
                  [](unsigned char c) { return std::tolower(c); });

   // Determine the appropriate enum to use based on the name
   // Check most likely options first

   // Box rearranger
   if (RearrComp == "box") {
      ReturnRearr = RearrBox;

      // Subset rearranger
   } else if (RearrComp == "subset") {
      ReturnRearr = RearrSubset;

      // Default
   } else if (RearrComp == "default") {
      ReturnRearr = RearrDefault;

   } else {
      ReturnRearr = RearrUnknown;
   }

   return ReturnRearr;

} // End RearrFromString

//------------------------------------------------------------------------------
// Converts string choice for File Format to an enum
FileFmt
FileFmtFromString(const std::string &Format // [in] choice of IO file format
) {
   // Set default return value
   FileFmt ReturnFileFmt = FmtUnknown;

   // Convert input string to lowercase for easier comparison
   std::string FmtCompare = Format;
   std::transform(FmtCompare.begin(), FmtCompare.end(), FmtCompare.begin(),
                  [](unsigned char c) { return std::tolower(c); });

   // Determine the appropriate enum to use based on the input string
   // Check most likely options first

   // NetCDF4 variants should be default so check first
   // we assume netcdf4 refers to netcdf4p
   if (FmtCompare == "netcdf4") {
      ReturnFileFmt = FmtNetCDF4;

      // Check for ADIOS
   } else if (FmtCompare == "adios") {
      ReturnFileFmt = FmtADIOS;

      // Check specific netcdf4 variants - compressed
   } else if (FmtCompare == "netcdf4c") {
      ReturnFileFmt = FmtNetCDF4c;

      // Check specific netcdf4 variants - parallel
   } else if (FmtCompare == "netcdf4p") {
      ReturnFileFmt = FmtNetCDF4p;

      // Check for older netcdf
   } else if (FmtCompare == "netcdf3") {
      ReturnFileFmt = FmtNetCDF3;

      // Check for older pnetcdf
   } else if (FmtCompare == "pnetcdf") {
      ReturnFileFmt = FmtPnetCDF;

      // Check for native HDF5
   } else if (FmtCompare == "hdf5") {
      ReturnFileFmt = FmtHDF5;

   } else if (FmtCompare == "default") {
      ReturnFileFmt = FmtDefault;

   } else {
      ReturnFileFmt = FmtUnknown;
   }

   return ReturnFileFmt;

} // end of FileFmtFromString

//------------------------------------------------------------------------------
// Converts string choice for IO read/write mode to an enum
Mode ModeFromString(
    const std::string &ModeChoice // [in] IO mode choice (read/write)
) {
   // Set default return value
   Mode ReturnMode = ModeUnknown;

   // Convert input string to lowercase for easier comparison
   std::string ModeComp = ModeChoice;
   std::transform(ModeComp.begin(), ModeComp.end(), ModeComp.begin(),
                  [](unsigned char c) { return std::tolower(c); });

   // Determine the appropriate enum to use based on the input string

   // Read only
   if (ModeComp == "read") {
      ReturnMode = ModeRead;

      // Write only
   } else if (ModeComp == "write") {
      ReturnMode = ModeWrite;

   } else {
      ReturnMode = ModeUnknown;
   }

   return ReturnMode;

} // End ModeFromString

//------------------------------------------------------------------------------
// Converts string choice for existence behavior to an enum
IfExists IfExistsFromString(
    const std::string &InIfExists // [in] choice of behavior on file existence
) {
   // Set default return value
   IfExists ReturnIfExists = IfExists::Fail;

   // Convert input string to lowercase for easier comparison
   std::string IECompare = InIfExists;
   std::transform(IECompare.begin(), IECompare.end(), IECompare.begin(),
                  [](unsigned char c) { return std::tolower(c); });

   // Determine the appropriate enum to use based on the input string

   // Fail with error
   if (IECompare == "fail") {
      ReturnIfExists = IfExists::Fail;

      // Replace existing file
   } else if (IECompare == "replace") {
      ReturnIfExists = IfExists::Replace;

      // Append or insert into existing file
   } else if (IECompare == "append") {
      ReturnIfExists = IfExists::Append;

   } else {
      ReturnIfExists = IfExists::Fail;
   }

   return ReturnIfExists;

} // End IfExistsFromString

//------------------------------------------------------------------------------
// Converts string choice for floating point precision to an enum
Precision PrecisionFromString(
    const std::string &InPrecision // [in] choice of floating point precision
) {
   // Set default return value
   Precision ReturnPrec = Precision::Double;

   // Convert input string to lowercase for easier comparison
   std::string PrecComp = InPrecision;
   std::transform(PrecComp.begin(), PrecComp.end(), PrecComp.begin(),
                  [](unsigned char c) { return std::tolower(c); });

   // Determine the appropriate enum to use based on the string name

   // Maintain full double precision
   if (PrecComp == "double") {
      ReturnPrec = Precision::Double;

      // Write reduced (single) precision
   } else if (PrecComp == "single") {
      ReturnPrec = Precision::Single;

   } else {
      ReturnPrec = Precision::Double;
   }

   return ReturnPrec;

} // End PrecisionFromString

// Methods
//------------------------------------------------------------------------------
// Initializes the IO system based on configuration inputs and
// default MPI communicator
int init(const MPI_Comm &InComm // [in] MPI communicator to use
) {

   int Err = 0; // success error code
   // Retrieve parallel IO parameters from the Omega configuration
   // TODO: replace with actual config, for now hardwired
   // extern int SysID;

   FileFmt DefaultFileFmt = FileFmtFromString("netcdf4c");
   int NumIOTasks         = 1;
   int IOStride           = 1;
   int IOBaseTask         = 0;
   Rearranger Rearrange   = RearrDefault;

   // Call PIO routine to initialize
   DefaultRearr = Rearrange;
   Err          = PIOc_Init_Intracomm(InComm, NumIOTasks, IOStride, IOBaseTask,
                                      Rearrange, &SysID);
   if (Err != 0)
      LOG_ERROR("IO::init: Error initializing SCORPIO");

   return Err;

} // end init

//------------------------------------------------------------------------------
// This routine opens a file for reading or writing, depending on the
// Mode argument. The filename with full path must be supplied and
// a FileID is returned to be used by other IO functions.
// The format of the file is assumed to be the default defined on init
// but can be optionally changed through this open function.
// For files to be written, optional arguments govern the behavior to be
// used if the file already exists, and the precision of any floating point
// variables. Returns an error code.
int openFile(
    int &FileID,                 // [out] returned fileID for this file
    const std::string &Filename, // [in] name (incl path) of file to open
    Mode InMode,                 // [in] mode (read or write)
    FileFmt InFormat,            // [in] (optional) file format
    IfExists InIfExists,         // [in] (for writes) behavior if file exists
    Precision InPrecision        // [in] (for writes) precision of floats
) {

   int Err    = 0;        // default success return code
   int Format = InFormat; // coerce to integer for PIO calls

   switch (InMode) {

   // If reading, open the file for read-only
   case ModeRead:
      Err = PIOc_openfile(SysID, &FileID, &Format, Filename.c_str(), InMode);
      if (Err != PIO_NOERR)
         LOG_ERROR("IO::openFile: PIO error opening file {} for read",
                   Filename);
      break;

   // If writing, the open will depend on what to do if the file
   // exists.
   case ModeWrite:

      switch (InIfExists) {
      // If the write should be a new file and fail if the
      // file exists, we use create and fail with an error
      case IfExists::Fail:
         Err = PIOc_createfile(SysID, &FileID, &Format, Filename.c_str(),
                               NC_NOCLOBBER | InMode);
         if (Err != PIO_NOERR)
            LOG_ERROR("IO::openFile: PIO error opening file {} for writing",
                      Filename);
         break;

      // If the write should replace any existing file
      // we use create with the CLOBBER option
      case IfExists::Replace:
         Err = PIOc_createfile(SysID, &FileID, &Format, Filename.c_str(),
                               NC_CLOBBER | InMode);
         if (Err != PIO_NOERR)
            LOG_ERROR("IO::openFile: PIO error opening file {} for writing",
                      Filename);
         break;

      // If the write should append or add to an existing file
      // we open the file for writing
      case IfExists::Append:
         Err = PIOc_openfile(SysID, &FileID, &Format, Filename.c_str(), InMode);
         if (Err != PIO_NOERR)
            LOG_ERROR("IO::openFile: PIO error opening file {} for writing",
                      Filename);
         break;

      default:
         LOG_ERROR("IO::openFile: unknown IfExists option for writing");

      } // end switch IfExists
      break;

   // Unknown mode
   default:
      LOG_ERROR("IO::fileOpen: Unknown Mode for file");

   } // End switch on Mode

   return Err;

} // End openFile
//------------------------------------------------------------------------------
// Closes an open file using the fileID, returns an error code
int closeFile(int &FileID /// [in] ID of the file to be closed
) {
   // Just calls the PIO close routine
   int Err = PIOc_closefile(FileID);
   return Err;

} // End closeFile

//------------------------------------------------------------------------------
// Retrieves a dimension length from an input file, given the name
// of the dimension. Returns the length if exists, but returns a negative
// value if a dimension of that length is not found in the file.
int getDimLength(int FileID, // [in] ID of the file containing dim
                 const std::string &DimName // [in] name of dimension
) {

   int Err = 0; // Local error code

   // First get the dimension ID
   int DimID = 0;
   Err       = PIOc_inq_dimid(FileID, DimName.c_str(), &DimID);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::getDimLength: PIO error while retrieving dimensionID");
      return -1;
   }

   // Now retrieve the length and return
   PIO_Offset Length = -1;
   Err               = PIOc_inq_dimlen(FileID, DimID, &Length);
   if (Err != PIO_NOERR) {
      LOG_ERROR(
          "IO::getDimLength: PIO error while retrieving dimension length");
      return -1;
   }
   return Length;

} // End getDimLength

//------------------------------------------------------------------------------
// Defines a dimension for an output file. Returns a dimension id to
// be used in future field definitions as well as an error flag.
int defineDim(int FileID,                 // [in] ID of the file containing dim
              const std::string &DimName, // [in] name of dimension
              int Length,                 // [in] length of dimension
              int &DimID                  // [out] dimension id assigned
) {

   int Err = 0;

   Err = PIOc_def_dim(FileID, DimName.c_str(), Length, &DimID);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::defineDim: PIO error while defining dimension");
      Err = -1;
   }

   return Err;

} // End defineDim

//------------------------------------------------------------------------------
// Writes metadata (name, value) associated with a variable and/or file.
// The variable ID can be GlobalID for global file and/or simulation
// metadata. This interface and PIO use a void pointer to create a generic
// interface.
int writeMeta(const std::string &MetaName, // [in] name of metadata
              I4 MetaValue,                // [in] value of metadata
              int FileID,                  // [in] ID of the file for writing
              int VarID // [in] ID for variable associated with metadata
) {

   int Err             = 0;
   IODataType MetaType = IOTypeI4;
   PIO_Offset Length   = 1;

   Err = PIOc_put_att(FileID, VarID, MetaName.c_str(), MetaType, Length,
                      &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::writeMeta(I4): PIO error while writing metadata");
      Err = -1;
   }

   return Err;

} // End writeMeta (I4)

int writeMeta(const std::string &MetaName, // [in] name of metadata
              I8 MetaValue,                // [in] value of metadata
              int FileID,                  // [in] ID of the file for writing
              int VarID // [in] ID for variable associated with metadata
) {

   int Err             = 0;
   IODataType MetaType = IOTypeI8;
   PIO_Offset Length   = 1;

   Err = PIOc_put_att(FileID, VarID, MetaName.c_str(), MetaType, Length,
                      &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::writeMeta(I8): PIO error while writing metadata");
      Err = -1;
   }

   return Err;

} // End writeMeta (I8)

int writeMeta(const std::string &MetaName, // [in] name of metadata
              R4 MetaValue,                // [in] value of metadata
              int FileID,                  // [in] ID of the file for writing
              int VarID // [in] ID for variable associated with metadata
) {

   int Err             = 0;
   IODataType MetaType = IOTypeR4;
   PIO_Offset Length   = 1;

   Err = PIOc_put_att(FileID, VarID, MetaName.c_str(), MetaType, Length,
                      &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::writeMeta(R4): PIO error while writing metadata");
      Err = -1;
   }

   return Err;

} // End writeMeta (R4)

int writeMeta(const std::string &MetaName, // [in] name of metadata
              R8 MetaValue,                // [in] value of metadata
              int FileID,                  // [in] ID of the file for writing
              int VarID // [in] ID for variable associated with metadata
) {

   int Err             = 0;
   IODataType MetaType = IOTypeR8;
   PIO_Offset Length   = 1;

   Err = PIOc_put_att(FileID, VarID, MetaName.c_str(), MetaType, Length,
                      &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::writeMeta(R8): PIO error while writing metadata");
      Err = -1;
   }

   return Err;

} // End writeMeta (R8)

int writeMeta(const std::string &MetaName, // [in] name of metadata
              std::string MetaValue,       // [in] value of metadata
              int FileID,                  // [in] ID of the file for writing
              int VarID // [in] ID for variable associated with metadata
) {

   int Err             = 0;
   IODataType MetaType = IOTypeChar;
   PIO_Offset Length   = MetaValue.length() + 1; // add 1 for char terminator

   Err = PIOc_put_att(FileID, VarID, MetaName.c_str(), MetaType, Length,
                      (void *)MetaValue.c_str());
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::writeMeta(str): PIO error while writing metadata");
      Err = -1;
   }

   return Err;

} // End writeMeta (str)

//------------------------------------------------------------------------------
// Reads metadata (name, value) associated with a variable and/or file.
// The variable ID can be GlobalID for global file and/or simulation
// metadata. Specific interfaces for supported data types are aliased to the
// same generic form.
int readMeta(const std::string &MetaName, // [in] name of metadata
             I4 &MetaValue,               // [out] value of metadata
             int FileID,                  // [in] ID of the file for writing
             int VarID // [in] ID for variable associated with metadata
) {
   int Err = 0;

   Err = PIOc_get_att(FileID, VarID, MetaName.c_str(), &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::readMeta(I4): PIO error while reading metadata");
      Err = -1;
   }

   return Err;

} // End readMeta (I4)

int readMeta(const std::string &MetaName, // [in] name of metadata
             I8 &MetaValue,               // [out] value of metadata
             int FileID,                  // [in] ID of the file for writing
             int VarID // [in] ID for variable associated with metadata
) {
   int Err = 0;

   Err = PIOc_get_att(FileID, VarID, MetaName.c_str(), &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::readMeta(I8): PIO error while reading metadata");
      Err = -1;
   }

   return Err;

} // End readMeta (I8)

int readMeta(const std::string &MetaName, // [in] name of metadata
             R4 &MetaValue,               // [out] value of metadata
             int FileID,                  // [in] ID of the file for writing
             int VarID // [in] ID for variable associated with metadata
) {
   int Err = 0;

   Err = PIOc_get_att(FileID, VarID, MetaName.c_str(), &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::readMeta(R4): PIO error while reading metadata");
      Err = -1;
   }

   return Err;

} // End readMeta (R4)

int readMeta(const std::string &MetaName, // [in] name of metadata
             R8 &MetaValue,               // [out] value of metadata
             int FileID,                  // [in] ID of the file for writing
             int VarID // [in] ID for variable associated with metadata
) {
   int Err = 0;

   Err = PIOc_get_att(FileID, VarID, MetaName.c_str(), &MetaValue);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::readMeta(R8): PIO error while reading metadata");
      Err = -1;
   }

   return Err;

} // End readMeta (R8)

int readMeta(const std::string &MetaName, // [in] name of metadata
             std::string &MetaValue,      // [out] value of metadata
             int FileID,                  // [in] ID of the file for writing
             int VarID // [in] ID for variable associated with metadata
) {
   int Err = 0;

   // For string variables, find the length of the string first
   // and resize the string to make sure the allocated length is large
   // enough when passing the pointer later
   PIO_Offset Length;
   Err = PIOc_inq_attlen(FileID, VarID, MetaName.c_str(), &Length);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::readMeta(str): PIO error while finding string length");
      Err = -1;
      return Err;
   }
   int StrLength = Length - 1;
   MetaValue.resize(StrLength);

   // Now read the string
   Err =
       PIOc_get_att(FileID, VarID, MetaName.c_str(), (void *)MetaValue.c_str());
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::readMeta(str): PIO error while reading metadata");
      Err = -1;
   }

   return Err;

} // End readMeta (str)

//------------------------------------------------------------------------------
// Defines a variable for an output file. The name and dimensions of
// the variable must be supplied. An ID is assigned to the variable
// for later use in the writing of the variable.
int defineVar(int FileID,                 // [in] ID of the file containing dim
              const std::string &VarName, // [in] name of variable
              IODataType VarType,         // [in] data type for the variable
              int NDims,                  // [in] number of dimensions
              int *DimIDs,                // [in] vector of NDims dimension IDs
              int &VarID                  // [out] id assigned to this variable
) {

   int Err = 0;

   Err = PIOc_def_var(FileID, VarName.c_str(), VarType, NDims, DimIDs, &VarID);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::defineVar: PIO error while defining variable");
      Err = -1;
   }

   return Err;

} // End defineVar

//------------------------------------------------------------------------------
/// Ends define phase signifying all field definitions and metadata
/// have been written and the larger data sets can now be written
int endDefinePhase(int FileID ///< [in] ID of the file being written
) {
   int Err = 0;

   Err = PIOc_enddef(FileID);
   if (Err != PIO_NOERR) {
      LOG_ERROR("IO::endDefinePhase: PIO error in enddef");
      Err = -1;
   }

   return Err;

} // End endDefinePhase

//------------------------------------------------------------------------------
// Creates a PIO decomposition description to describe the layout of
// a distributed array of given type.
int createDecomp(
    int &DecompID,      // [out] ID assigned to the new decomposition
    IODataType VarType, // [in] data type of array
    int NDims,          // [in] number of array dimensions
    const std::vector<int> &DimLengths, // [in] global dimension lengths
    int Size,                           // [in] local size of array
    const std::vector<int> &GlobalIndx, // [in] global indx for each local indx
    Rearranger Rearr                    // [in] rearranger method to use
) {

   int Err = 0; // default return code

   // Convert global index array into an offset array expected by PIO
   std::vector<PIO_Offset> CompMap;
   CompMap.resize(Size);
   for (int i = 0; i < Size; ++i) {
      CompMap[i] = GlobalIndx[i];
   } // end global index loop

   // Call the PIO routine to define the decomposition
   // int TmpRearr = Rearr; // needed for type compliance across interface
   // Err = PIOc_InitDecomp(SysID, VarType, NDims, DimLengths, Size, CompMap,
   //                      &DecompID, &TmpRearr, nullptr, nullptr);
   Err = PIOc_init_decomp(SysID, VarType, NDims, &DimLengths[0], Size,
                          &CompMap[0], &DecompID, Rearr, nullptr, nullptr);
   if (Err != PIO_NOERR)
      LOG_ERROR("IOCreateDecomp: PIO error defining decomposition");

   return Err;

} // End createDecomp

//------------------------------------------------------------------------------
// Removes a defined PIO decomposition description to free memory
int destroyDecomp(int &DecompID // [inout] ID for decomposition to be removed
) {

   int Err = PIOc_freedecomp(SysID, DecompID);
   if (Err != PIO_NOERR)
      LOG_ERROR("IODestroyDecomp: PIO error freeing decomposition");

   return Err;

} // End destroyDecomp

//------------------------------------------------------------------------------
// Reads a distributed array. Uses a void pointer for generic interface.
// All arrays are assumed to be in contiguous storage.
int readArray(void *Array,                // [out] array to be read
              int Size,                   // [in] local size of array
              const std::string &VarName, // [in] name of variable to read
              int FileID,                 // [in] ID of open file to read from
              int DecompID,               // [in] decomposition ID for this var
              int &VarID // [out] Id assigned to variable for later use
) {

   int Err = 0; // default return code

   // Find variable ID from file
   Err = PIOc_inq_varid(FileID, VarName.c_str(), &VarID);
   if (Err != PIO_NOERR)
      LOG_ERROR("IOReadArray(I4): Error finding varid");

   // PIO Read array call to read the distributed array
   PIO_Offset ASize = Size;
   Err              = PIOc_read_darray(FileID, VarID, DecompID, ASize, Array);
   if (Err != PIO_NOERR)
      LOG_ERROR("IOReadArray(I4): Error in SCORPIO read array");

   return Err;

} // End IOReadArray

//------------------------------------------------------------------------------
// Writes a distributed array. This generic interface uses void pointers.
// All arrays are assumed to be in contiguous storage and the variable
// must have a valid ID assigned by the defineVar function.

int writeArray(void *Array,     // [in] array to be written
               int Size,        // [in] size of array to be written
               void *FillValue, // [in] value to use for missing entries
               int FileID,      // [in] ID of open file to write to
               int DecompID,    // [in] decomposition ID for this var
               int VarID        // [in] variable ID assigned by defineVar
) {
   int Err = 0;

   PIO_Offset Asize = Size;

   Err = PIOc_write_darray(FileID, VarID, DecompID, Asize, Array, FillValue);

   return Err;

} // end writeArray

//------------------------------------------------------------------------------

} // end namespace IO
} // end namespace OMEGA

//===----------------------------------------------------------------------===//
