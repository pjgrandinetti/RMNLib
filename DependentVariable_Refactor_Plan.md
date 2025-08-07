# DependentVariable.c Refactoring Plan

## Current State
- **File**: `src/core/DependentVariable.c` (4,990 lines)
- **Functions**: 60+ public functions, 10+ internal/static functions
- **Status**: ✅ All tests passing, clean git state

## Critical Issue: Core Infrastructure Functions
**All `impl_` functions and core infrastructure MUST stay together:**
1. `impl_DependentVariableFinalize` - destructor callback
2. `impl_DependentVariableEqual` - equality callback  
3. `impl_DependentVariableCopyFormattingDesc` - formatting callback
4. `impl_DependentVariableCreateJSON` - JSON callback
5. `impl_DependentVariableDeepCopy` - deep copy callback
6. `impl_DependentVariableCreate` - main creation function (called by 9 functions)
7. `impl_InitDependentVariableFields` - field initialization helper
8. `DependentVariableAllocate` - allocation helper (uses all callbacks)

**All creation functions that depend on these MUST be in the same module.**

## Proposed Module Structure

### **Module 1: `DependentVariable_core.c`** (~1800 lines)
**Purpose**: Core infrastructure, type registration, ALL creation functions, and serialization
**Location**: `src/core/dependent_variable/DependentVariable_core.c` (new file)
- **Core infrastructure**: 
  - `impl_DependentVariableFinalize`, `impl_DependentVariableEqual`
  - `impl_DependentVariableCopyFormattingDesc`, `impl_DependentVariableCreateJSON`
  - `impl_DependentVariableDeepCopy`, `impl_DependentVariableCreate`
  - `impl_InitDependentVariableFields`, `DependentVariableAllocate`
- **Type registration**: `DependentVariableGetTypeID`
- **Creation functions**: 
  - `DependentVariableCreate`, `DependentVariableCreateWithComponentsNoCopy`
  - `DependentVariableCreateWithSize`, `DependentVariableCreateDefault`
  - `DependentVariableCreateWithComponent`, `DependentVariableCreateExternal`
  - `DependentVariableCreateMinimal`, `DependentVariableCreateFromDictionary`
  - `DependentVariableCreateFromJSON`
- **Copy functions**: `DependentVariableCopy`, `DependentVariableCreateComplexCopy`
- **JSON/Dictionary serialization**: 
  - `DependentVariableDictionaryCreateFromJSON`, `DependentVariableCopyAsDictionary`
- **Core helper**: `DependentVariableComponentsCountFromQuantityType` (used by creation functions)

### **Module 2: `DependentVariable_accessors.c`** (~700 lines)  
**Purpose**: Property getters and setters, including type queries
**Location**: `src/core/dependent_variable/DependentVariable_accessors.c` (move existing file here)
- **Basic accessors**: 
  - `DependentVariableGetName`, `DependentVariableSetName`
  - `DependentVariableGetDescription`, `DependentVariableSetDescription`
  - `DependentVariableGetType`, `DependentVariableSetType`
  - `DependentVariableGetEncoding`, `DependentVariableSetEncoding`
  - `DependentVariableGetComponentsURL`, `DependentVariableSetComponentsURL`
- **Quantity accessors**: 
  - `DependentVariableGetQuantityName`, `DependentVariableSetQuantityName`
  - `DependentVariableGetQuantityType`, `DependentVariableSetQuantityType`
  - `DependentVariableGetNumericType`, `DependentVariableSetNumericType`
- **Metadata accessors**: 
  - `DependentVariableGetApplicationMetaData`, `DependentVariableSetApplicationMetaData`
  - `DependentVariableGetOwner`, `DependentVariableSetOwner`
  - `DependentVariableGetSparseSampling`, `DependentVariableSetSparseSampling`
- **Size/Count accessors**: 
  - `DependentVariableGetSize`, `DependentVariableSetSize`
  - `DependentVariableGetComponentCount`
- **Type queries**: 
  - `DependentVariableIsScalarType`, `DependentVariableIsVectorType`
  - `DependentVariableIsPixelType`, `DependentVariableIsMatrixType`
  - `DependentVariableIsSymmetricMatrixType`
- **Type helpers**: `DependentVariableShouldSerializeExternally`
- **Additional helpers**: `DependentVariableCreateQuantityTypesArray`

### **Module 3: `DependentVariable_components.c`** (~800 lines)
**Purpose**: Component data management and processing
**Location**: `src/core/dependent_variable/DependentVariable_components.c` (new file)
- **Component array operations**: 
  - `DependentVariableGetComponents`, `DependentVariableSetComponents`
  - `DependentVariableCopyComponents`
- **Individual component access**: 
  - `DependentVariableGetComponentAtIndex`, `DependentVariableSetComponentAtIndex`
  - `DependentVariableInsertComponentAtIndex`, `DependentVariableRemoveComponentAtIndex`
- **Component labels**: 
  - `DependentVariableGetComponentLabels`, `DependentVariableSetComponentLabels`
  - `DependentVariableCreateComponentLabelForIndex`
  - `DependentVariableGetComponentLabelAtIndex`, `DependentVariableSetComponentLabelAtIndex`
- **Low-level value access**: 
  - `DependentVariableGetFloatValueAtMemOffset`, `DependentVariableGetDoubleValueAtMemOffset`
  - `DependentVariableGetFloatComplexValueAtMemOffset`, `DependentVariableGetDoubleComplexValueAtMemOffset`
  - `DependentVariableGetFloatValueAtMemOffsetForPart`, `DependentVariableGetDoubleValueAtMemOffsetForPart`
  - `DependentVariableCreateValueFromMemOffset`, `DependentVariableSetValueAtMemOffset`
  - `DependentVariableSetValues`
- **Component data processing**: 
  - `DependentVariableCreateCSDMComponentsData`, `DependentVariableCreatePackedSparseComponentsArray`

### **Module 4: `DependentVariable_operations.c`** (~1100 lines)
**Purpose**: Mathematical operations and data manipulation
**Location**: `src/core/dependent_variable/DependentVariable_operations.c` (new file)
- **Basic arithmetic**: 
  - `DependentVariableAdd`, `DependentVariableSubtract`
  - `DependentVariableMultiply`, `DependentVariableDivide`
- **Scalar operations**: 
  - `DependentVariableMultiplyValuesByDimensionlessRealConstant`
  - `DependentVariableMultiplyValuesByDimensionlessComplexConstant`
- **Complex operations**: 
  - `DependentVariableTakeComplexPart`, `DependentVariableConjugate`
  - `DependentVariableCombineMagnitudeWithArgument`
- **Utility operations**: 
  - `DependentVariableSetValuesToZero`, `DependentVariableTakeAbsoluteValue`
  - `DependentVariableZeroPartInRange`
- **Data mutation**: 
  - `DependentVariableAppend`, `DependentVariableConvertToUnit`
- **Helper functions**: 
  - `perform_arithmetic_with_conversion`, `perform_arithmetic_elementwise`

### **Module 5: `DependentVariable_dimensions.c`** (~200 lines)
**Purpose**: Dimension-related operations and format conversion functions
**Location**: `src/core/dependent_variable/DependentVariable_dimensions.c` (new file)
- **Cross-section**: `DependentVariableCreateCrossSection`

### **Remaining in main file**: `DependentVariable.c` (~100 lines)
**Location**: `src/core/DependentVariable.c` (existing file, heavily reduced)
- Header includes and struct definition (`struct impl_DependentVariable`)
- Any remaining static helpers that don't fit elsewhere

## Directory Structure After Refactoring
```
src/core/
├── DependentVariable.c          (reduced to ~100 lines)
├── DependentVariable.h          (unchanged)
├── Dataset.c
├── Dataset.h  
├── Dimension.c
├── Dimension.h
├── dependent_variable/          (NEW FOLDER)
│   ├── DependentVariable_core.c
│   ├── DependentVariable_accessors.c
│   ├── DependentVariable_components.c
│   ├── DependentVariable_operations.c
│   └── DependentVariable_dimensions.c
└── ... (other core files)
```

**Future expansion possibilities:**
- `src/core/dataset/` for Dataset modularization
- `src/core/dimension/` for Dimension modularization

## Implementation Strategy

### **Phase 1**: Prepare core module (1 hour)
1. Keep ALL `impl_` functions together - do NOT make them non-static
2. They stay as internal implementation details within the core module
3. Test that current structure still works

### **Phase 2**: Extract one module at a time (5-7 hours)
1. **Start with accessors** (simple getters/setters)
2. **Then components** (depends on accessors)
3. **Then operations** (depends on components, includes arithmetic and mutation)
4. **Then dimensions** (dimension-related operations)
5. **Core module stays untouched** (everything depends on it)

### **Phase 3**: Clean up and validate (1 hour)
- Ensure all 60+ functions are present
- Run full test suite
- Check for any missing exports

## Safety Measures
- ✅ Extract ONE function at a time
- ✅ Build + test after each function move
- ✅ Git commit after each successful extraction
- ✅ Immediate rollback on any failure
- ✅ Function count validation script

