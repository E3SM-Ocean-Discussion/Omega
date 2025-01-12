(omega-dev-config)=

## Model Configuration (Config)

Model configuration refers to all of the necessary variables to configure
and run the model. It contains all the input parameters, choices of methods,
physical coefficients for parameterizations and various options for I/O.
Typically, this is read as a single input file that can serve as
a description of the configuration for provenance as well. The input YAML
file and it's requirements are described in the
[User Guide](#omega-user-config).

Within the model, the paradigm should be that a given module will extract
its configuration from the full configuration as part of the module's
initialization and store it in private class or module variables to be used
later. This is partly for performance reasons. In the current implementation,
the configuration is read in and owned by the master MPI task and the
retrieval from the full configuration into module variables involves a
broadcast to the rest of the tasks. In addition, this paradigm will allow
the deletion of the configuration at the end of initialization to free memory.

The configuration is built around the yaml-cpp library. Each configuration
contains a ``YAML::Node`` and a name for the configuration.
Most of the functions below are wrappers around yaml-cpp templated functions.
Rather than using templated forms in omega, we alias the function interfaces
so that the syntax is cleaner.

Within Omega, the full configuration file is read just after the MachEnv
initialization using:
```c++
OMEGA::Config::readAll("omega.yml");
```
The full Omega configuration is stored in a static variable for later
retrievals.

Each module in Omega will extract its own configuration variables by
first retrieving the stored Omega configuration, then retrieving the module
configuration (multiple times if the module is nested under a parent)
and finally retrieving the variable by name. The sequence might look like
this for the example shown in the User Guide:
```c++
// Get pointer to full configuration
OMEGA::Config *OmegaConfig = OMEGA::Config::getOmegaConfig;

OMEGA::Config HmixConfig; // creates an empty config for Hmix

OmegaConfig->get("Hmix",HmixConfig); // extracts all Hmix config vars from omega

// Get individual variables
Err = HmixConfig.get("HmixScaleWithMesh", HmixScaleWithMesh);
Err = HmixConfig.get("MaxMeshDensity"   , MaxMeshDensity);
Err = HmixConfig.get("HmixUseRefWidth"  , HmixUseRefWidth);
Err = HmixConfig.get("HmixRefWidth"     , HmixRefWidth);
```
If the module is deeper in the hierarchy (eg an HmixDel2 module
under the Hmix module), you would need an additional call to extract
the subconfiguration, like:
```c++
OMEGA::Config HmixDel2Config; // creates and empty config for Hmix Del2
Err = HmixConfig.get(HmixDel2Config);
```
If the variable cannot be found in the input configuration, a non-zero
error code is returned.  Existence can also be checked with the bool
functions existsGroup or existsVar, where the variable name is provided.
The developer will need to decide the action to take based on that
condition. For example, the code could return a critical error and terminate
if it is required. In some cases, it may be sufficient to issue a warning
and set the value of the variable to an internal default.
In this latter case, it is a good idea to add the variable and its value to
the configuration using the add function. For example:
```c++
Err = HmixConfig.add("HmixNewVar", value);
```
Then it will be included in any future writing of the configuration in
order to save provenance. Similarly, an existing value can be changed using
the ``set`` function that has an analogous form. There is also a remove
function that can remove a variable or subconfiguration from any Config -
only the name needs to be supplied. Finally, any configuration can be
written using the write function with a filename supplied as in
```c++
Err = OmegaConfig.write("FileName");
```

While the focus is on reading a configuration, a new configuration can be
built up from scratch by creating an empty Config instance using the
constructors and adding variables and sub-configurations through the add
functions. In this process, the configuration must be built up from the
bottom of the hierarchy and added to each higher layer until reaching the
top. For an example of this process, see the ConfigTest unit test that
builds a sample config for testing.

In order to support the automatic generation of a default Config file,
we request all modules include the default configuration as comments in
the header file, using a backslash ConfigInput to delimit the lines to
extract into a default Config input. For example:
```c++
/// \ConfigInput
/// #
/// # Group description (eg. hmix: the horizontal mix configuration)
/// #
/// groupName:
///    #
///    # Parameter description (eg horizontal mixing coeff)
///    #    more description (units, acceptable values or range)
///    #
///    varName1: defaultValue1
///    #
///    # Parameter description
///    #
///    varName2: defaultValue2
///    [ continue for remaining vars in this block]
///
/// \EndConfigInput (might not be necessary?)
```
The automated tool (not yet developed) can extract these into a default
input config file and optionally remove the comment lines for a more
compact input. User's can then modify this default file to configure their
specific simulation.
