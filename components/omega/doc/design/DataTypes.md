(omega-design-data-types)=
# DataTypes

## 1 Overview

Data type aliases are a useful means for managing precision within OMEGA and
to improve readability for for YAKL array data types. This header file
defines a number of aliases to common data types used throughout OMEGA.


## 2 Requirements


### 2.1 Requirement: Both flexible and fixed floating point

We wish to explore mixed precision and lower precision floating point within
OMEGA so require both a generic floating point type that can be switched at
compile time as well as explicit single and double-precision (32-bit, 64-bit)
types to enforce precision where needed.

### 2.2 Desired: Readability for array types

OMEGA will be using YAKL array data types which have a long syntax. For
readability, these should be aliased to a shorter, intuitive data type.

### 2.3 Desired: Fixed width integer types

For better control over integer storage, it might be better to define
fixed-width integer data types rather than the generic int, long.

### 2.4 Desired: Interoperability

As much as possible, data types should not inhibit interoperability
with other codes (eg E3SM coupler, 3rd-party libs) or frameworks
(eg other portability or AI/ML frameworks).

### 2.5 Future: Floating point conversion

Currently, language standard floating point conversion should be adequate,
but future mixed-precision implementations may require more reproducible
methods for implementation-dependent conversion.

## 3 Algorithmic Formulation

No algorithms needed yet, though reproducibility in type conversion may
require reproducible algorithms in the future.

## 4 Design

Currently, the data types can be defined in a single header file that sets
up aliases. Because the main data types are simply aliases to the standard
data types, interoperability (Req 2.4) should not be an issue. For the
YAKL arrays, YAKL provides means for interoperability with both other
languages and other frameworks and will be incorporated into any
interfaces with other codes.

### 4.1 Data types and parameters

#### 4.1.1 Parameters

We will use a global cpp define -D SINGLE_PRECISION at compile time as a
global switch for using single precision as the default real data type.
Otherwise, the default real will be double precision.


#### 4.1.2 Class/structs/data types

The data types will be defined using type aliases within a single header
file DataTypes.h We will use the "using" syntax rather than
the older typedef. For YAKL arrays, we require both device arrays (default)
and host array types and will use C-ordering.

```c++
// Standard integer and floating point types
using I4 = std::int32_t;
using I8 = std::int64_t;
using R4 = float;
using R8 = double;
#ifdef SINGLE_PRECISION
using Real = float;
#else
using Real = double;
#endif

// Aliases for YAKL arrays - by default on device and in
// C-ordering.
using Array1DI4   = YAKL::Array<I4,1,memDevice,styleC>
using Array1DI8   = YAKL::Array<I8,1,memDevice,styleC>
using Array1DR4   = YAKL::Array<R4,1,memDevice,styleC>
using Array1DR8   = YAKL::Array<R8,1,memDevice,styleC>
using Array1DReal = YAKL::Array<Real,1,memDevice,styleC>
using Array2DI4   = YAKL::Array<I4,2,memDevice,styleC>
using Array2DI8   = YAKL::Array<I8,2,memDevice,styleC>
using Array2DR4   = YAKL::Array<R4,2,memDevice,styleC>
using Array2DR8   = YAKL::Array<R8,2,memDevice,styleC>
using Array2DReal = YAKL::Array<Real,2,memDevice,styleC>
// continue this pattern for higher-dimensional arrays
// Also need similar aliases for arrays on the host
using ArrayHost1DI4   = YAKL::Array<I4,1,memHost,styleC>
// replicated as above for each type, dimension
```

### 4.2 Methods

No methods needed at this time.


## 5 Verification and Testing

Requirement 2.4 on interoperability will not have a unit test
and will be tested as part of larger system tests where coupling
with other codes is exercised.

### 5.1 Test explicit types

For the fixed types, verify the expected size using sizeof
  * tests requirement 2.3, 2.1 (explicit types)

### 5.2 Test global switch for default real size

Build test code with and without -D SINGLE_PRECISION and
verify size is as expected using sizeof
  * tests requirement 2.1

### 5.3 YAKL array test

Create YAKL arrays of each type on device. Create host arrays to
mirror each. Initialize YAKL arrays on device and copy to host
using YAKL. Create a non-YAKL array on host initialized the same
way and compare values from each on the host.
  * tests requirement 2.2
