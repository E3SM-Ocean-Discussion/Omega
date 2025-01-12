//===-- infra/Config.cpp - omega configuration implementation ----*- C++
//-*-===//
//
// The Config capability manages all the input parameters needed to configure
// and run a simulation using OMEGA. It reads an input configuration file
// in YAML format and stores it in an efficient way for retrieval by
// each module in OMEGA. It is based on the yaml-cpp library and relies on
// that library for all functionality.
//
//===----------------------------------------------------------------------===//

#include "Config.h"
#include "Broadcast.h"
#include "DataTypes.h"
#include "Logging.h"
#include "MachEnv.h"
#include "mpi.h"
#include "yaml-cpp/yaml.h"

#include <iostream>
#include <string>

namespace OMEGA {

// Declare some of the Config static variables
bool Config::IsMasterTask   = false;
bool Config::NotInitialized = true;
Config Config::ConfigAll;

//------------------------------------------------------------------------------
// Constructor that creates configuration with a given name and an emtpy
// YAML node that will be filled later with either a readAll or get call.
Config::Config(const std::string &InName // [in] name of config, node
) {

   // If this is the first Config created, set some of the static
   // variables
   if (NotInitialized) {
      // Determine master task and create a copy for all configs
      MachEnv *DefEnv      = MachEnv::getDefaultEnv();
      Config::IsMasterTask = DefEnv->isMasterTask();
   }

   // Set the name - this is also used as the name of the root YAML node
   // (a map node) in this configuration.
   Name = InName;

   // Create an empty node
   Node[Name];

} // end Config constructor

//------------------------------------------------------------------------------
// Constructor that creates a completely empty configuration. This should
// not be used except to initialize the static variable ConfigAll that
// is created before it can be properly filled.
Config::Config() {
   // Do nothing - class members will be filled later
} // end Config constructor

//------------------------------------------------------------------------------
// Reads the full configuration for omega and stores in it a static
// YAML node for later use.  The file must be in YAML format and must be in
// the same directory as the executable, though Unix soft links can be used
// to point to a file in an alternate location.  The full configuration is
// only read by the master task.  The accessor functions (get/set) will
// broadcast or communicate the data as needed to/from other MPI tasks.

int Config::readAll(const std::string ConfigFile // [in] input YAML config file
) {
   int Err = 0;

   // Now give the full config the omega name and extract the
   // top-level omega node from the Root.
   ConfigAll.Name = "omega";

   if (IsMasterTask) {
      // Read temporary root node
      YAML::Node RootNode = YAML::LoadFile(ConfigFile);
      // Extract Omega node
      ConfigAll.Node = RootNode["omega"];
   }

   return Err;

} // end Config::readAll

//------------------------------------------------------------------------------
// Retrieval (get) functions
//------------------------------------------------------------------------------
// Retrieves the top-level OMEGA config
Config *Config::getOmegaConfig() {

   Config *retConfig;

   if (Config::IsMasterTask) {
      retConfig = &ConfigAll;
   } else {
      retConfig = nullptr;
   }

   return retConfig;
}

//------------------------------------------------------------------------------
// Retrieves a sub-configuration from a parent Config. An empty SubConfig must
// have already been created with a name.
// Returns an error code that is non-zero if the group does not exist
int Config::get(Config &SubConfig // [inout] sub-configuration to retrieve
) {
   int Err = 0;
   if (Config::IsMasterTask) {
      std::string GroupName = SubConfig.Name;
      if (Node[GroupName]) { // the group exists
         SubConfig.Node = Node[GroupName];
      } else {
         LOG_ERROR("Config get group: could not find group {}", GroupName);
         Err = -1;
      }
   }
   Broadcast(Err);
   return Err;
}

//------------------------------------------------------------------------------
// Retrieves a 4-byte integer value from the Config based on name
// Returns a non-zero error code if the variable does not exist
int Config::get(const std::string VarName, // [in] name of variable to get
                I4 &Value                  // [out] value of the variable
) {
   int Err    = 0;
   int ValErr = -9999;

   // Extract variable from config on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Value = Node[VarName].as<OMEGA::I4>();
      } else {
         LOG_ERROR("Config get I4: could not find variable {}", VarName);
         Value = ValErr;
      }
   }

   // Broadcast the value to other tasks
   Broadcast(Value);
   if (Value == ValErr)
      Err = -1;

   return Err;
}

//------------------------------------------------------------------------------
// Retrieves an 8-byte integer value from the Config based on name
// Returns a non-zero error code if the variable does not exist
int Config::get(const std::string VarName, // [in] name of variable to get
                I8 &Value                  // [out] value of the variable
) {
   int Err   = 0;
   I8 ValErr = -99999;

   // Extract variable from config on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Value = Node[VarName].as<OMEGA::I8>();
      } else {
         LOG_ERROR("Config get I8: could not find variable {}", VarName);
         Value = ValErr;
      }
   }

   // Broadcast the value to other tasks
   Broadcast(Value);
   if (Value == ValErr)
      Err = -1;

   return Err;
}

//------------------------------------------------------------------------------
// Retrieves a 4-byte real value from the Config based on name
// Returns a non-zero error code if the variable does not exist
int Config::get(const std::string VarName, // [in] name of variable to get
                R4 &Value                  // [out] value of the variable
) {
   int Err   = 0;
   R4 ValErr = -999.999;

   // Extract variable from config on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Value = Node[VarName].as<OMEGA::R4>();
      } else {
         LOG_ERROR("Config get R4: could not find variable {}", VarName);
         Value = ValErr;
      }
   }

   // Broadcast the value to other tasks
   Broadcast(Value);
   if (Value == ValErr)
      Err = -1;

   return Err;
}

//------------------------------------------------------------------------------
// Retrieves an 8-byte real value from the Config based on name
// Returns a non-zero error code if the variable does not exist
int Config::get(const std::string VarName, // [in] name of variable to get
                R8 &Value                  // [out] value of the variable
) {
   int Err   = 0;
   R8 ValErr = -99999.999;

   // Extract variable from config on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Value = Node[VarName].as<OMEGA::R8>();
      } else {
         LOG_ERROR("Config get R8: could not find variable {}", VarName);
         Value = ValErr;
      }
   }

   // Broadcast the value to other tasks
   Broadcast(Value);
   if (Value == ValErr)
      Err = -1;

   return Err;
}

//------------------------------------------------------------------------------
// Retrieves a logical/boolean value from the Config based on name
// Returns a non-zero error code if the variable does not exist
int Config::get(const std::string VarName, // [in] name of variable to get
                bool &Value                // [out] value of the variable
) {
   int Err = 0;

   // Extract variable from config on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Value = Node[VarName].as<bool>();
      } else {
         LOG_ERROR("Config get bool: could not find variable {}", VarName);
         Err = -1;
      }
   }

   // Broadcast the value to other tasks
   Broadcast(Value);
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Retrieves a string value from the Config based on name
// Returns a non-zero error code if the variable does not exist
int Config::get(const std::string VarName, // [in] name of variable to get
                std::string &Value         // [out] value of the variable
) {
   int Err            = 0;
   std::string ValErr = "ConfigError";

   // Extract variable from config on master task
   std::string TmpVal;
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         TmpVal = Node[VarName].as<std::string>();
      } else {
         LOG_ERROR("Config get string: could not find variable {}", VarName);
         TmpVal = ValErr;
      }
   }

   // Broadcast the value to other tasks
   // Note that the broadcast will automatically resize TmpVal to fit on
   // non-master tasks
   Broadcast(TmpVal);
   Value = TmpVal;
   if (Value == ValErr)
      Err = -1;

   return Err;
}

//------------------------------------------------------------------------------
// Set functions
//------------------------------------------------------------------------------
// Resets the value of a variable in the config
// Returns a non-zero error code if variable is not found.
int Config::set(const std::string VarName, // [in] name of variable to set
                I4 Value                   // [in] new value of the variable
) {
   int Err = 0;

   // Config exists on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Node[VarName] = Value;
      } else {
         LOG_ERROR("Config set I4: could not find variable {}", VarName);
         Err = -1;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Resets the value of a variable in the config
// Returns a non-zero error code if variable is not found.
int Config::set(const std::string VarName, // [in] name of variable to set
                I8 Value                   // [in] new value of the variable
) {
   int Err = 0;

   // Config exists on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Node[VarName] = Value;
      } else {
         LOG_ERROR("Config set I8: could not find variable {}", VarName);
         Err = -1;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Resets the value of a variable in the config
// Returns a non-zero error code if variable is not found.
int Config::set(const std::string VarName, // [in] name of variable to set
                R4 Value                   // [in] new value of the variable
) {
   int Err = 0;

   // Config exists on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Node[VarName] = Value;
      } else {
         LOG_ERROR("Config set R4: could not find variable {}", VarName);
         Err = -1;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Resets the value of a variable in the config
// Returns a non-zero error code if variable is not found.
int Config::set(const std::string VarName, // [in] name of variable to set
                R8 Value                   // [in] new value of the variable
) {
   int Err = 0;

   // Config exists on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Node[VarName] = Value;
      } else {
         LOG_ERROR("Config set R8: could not find variable {}", VarName);
         Err = -1;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Resets the value of a variable in the config
// Returns a non-zero error code if variable is not found.
int Config::set(const std::string VarName, // [in] name of variable to set
                bool Value                 // [in] new value of the variable
) {
   int Err = 0;

   // Config exists on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Node[VarName] = Value;
      } else {
         LOG_ERROR("Config set bool: could not find variable {}", VarName);
         Err = -1;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Resets the value of a variable in the config
// Returns a non-zero error code if variable is not found.
int Config::set(const std::string VarName, // [in] name of variable to set
                std::string Value          // [in] new value of the variable
) {
   int Err = 0;

   // Config exists on master task
   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Node[VarName] = Value;
      } else {
         LOG_ERROR("Config set string: could not find variable {}", VarName);
         Err = -1;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Add functions
//------------------------------------------------------------------------------
// Adds a complete subconfiguration to an existing configuration
// Returns a non-zero error code if group already exists
int Config::add(const Config SubConfig // [in] Config to add
) {
   int Err = 0;

   std::string LocName = SubConfig.Name;
   if (Config::IsMasterTask) {
      if (Node[LocName]) { // the variable exists
         LOG_ERROR("Config add group: cannot add, group {} already exists",
                   LocName);
         Err = -1;
      } else {
         Node[LocName] = SubConfig.Node;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Adds a new variable to a configuration
// Returns a non-zero error code if variable already exists
int Config::add(const std::string VarName, // [in] name of variable to add
                I4 Value                   // [in] value of the variable
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         LOG_ERROR("Config add I4: variable {} already exists use set instead",
                   VarName);
         Err = -1;
      } else {
         Node[VarName] = Value;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Adds a new variable to a configuration
// Returns a non-zero error code if variable already exists
int Config::add(const std::string VarName, // [in] name of variable to add
                I8 Value                   // [in] value of the variable
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         LOG_ERROR("Config add I8: variable {} already exists use set instead",
                   VarName);
         Err = -1;
      } else {
         Node[VarName] = Value;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Adds a new variable to a configuration
// Returns a non-zero error code if variable already exists
int Config::add(const std::string VarName, // [in] name of variable to add
                R4 Value                   // [in] value of the variable
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         LOG_ERROR("Config add R4: variable {} already exists use set instead",
                   VarName);
         Err = -1;
      } else {
         Node[VarName] = Value;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Adds a new variable to a configuration
// Returns a non-zero error code if variable already exists
int Config::add(const std::string VarName, // [in] name of variable to add
                R8 Value                   // [in] value of the variable
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         LOG_ERROR("Config add R8: variable {} already exists use set instead",
                   VarName);
         Err = -1;
      } else {
         Node[VarName] = Value;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Adds a new variable to a configuration
// Returns a non-zero error code if variable already exists
int Config::add(const std::string VarName, // [in] name of variable to add
                bool Value                 // [in] value of the variable
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         LOG_ERROR(
             "Config add bool: variable {} already exists use set instead",
             VarName);
         Err = -1;
      } else {
         Node[VarName] = Value;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Adds a new variable to a configuration
// Returns a non-zero error code if variable already exists
int Config::add(const std::string VarName, // [in] name of variable to add
                std::string Value          // [in] value of the variable
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         LOG_ERROR("Config add string: variable {} already exists - use set",
                   VarName);
         Err = -1;
      } else {
         Node[VarName] = Value;
      }
   }
   Broadcast(Err);

   return Err;
}

//------------------------------------------------------------------------------
// Remove functions
//------------------------------------------------------------------------------
// Removes a variable from a configuration
int Config::remove(const std::string VarName // [in] name of variable to remove
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      if (Node[VarName]) { // the variable exists
         Node.remove(VarName);
      }
   }

   return Err;
}

//------------------------------------------------------------------------------
// Inquiry (existence) function
//------------------------------------------------------------------------------

// Checks to see if a configuration group exists in the configuration
bool Config::existsGroup(std::string GroupName // [in] name of group to find
) {
   bool result = false;
   if (Config::IsMasterTask) {
      if (Node[GroupName])
         result = true;
   }
   Broadcast(result);
   return result;
}

// Checks to see if a variable exists in the configuration
bool Config::existsVar(std::string VarName // [in] name of variable to find
) {
   bool result = false;
   if (Config::IsMasterTask) {
      if (Node[VarName])
         result = true;
   }
   Broadcast(result);
   return result;
}

//------------------------------------------------------------------------------
// Write function
//------------------------------------------------------------------------------
// Writes a configuration to a file
// Returns a non-zero error code if not successful
int Config::write(std::string FileName /// name of file for config
) {
   int Err = 0;

   if (Config::IsMasterTask) {
      std::ofstream Outfile(FileName);
      if (Outfile.good()) {
         Outfile << Node;
         Outfile.close();
      } else {
         Err = -1;
      }
   }

   Broadcast(Err);
   return Err;
}

} // end namespace OMEGA

//===----------------------------------------------------------------------===//
