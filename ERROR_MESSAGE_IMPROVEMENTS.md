# Error Message Improvements for DependentVariable JSON Parsing

## Overview
The error messages in `DependentVariableCreateFromJSON` and related functions have been significantly improved to provide more informative and user-friendly feedback when JSON parsing fails.

## Key Improvements

### 1. Context-Rich Error Messages
- **Before**: `"Missing or invalid \"type\""`
- **After**: `"Missing required 'type' field. Expected 'internal' or 'external' to specify how component data is stored"`

### 2. Specific Field Requirements
- **Before**: `"External DependentVariable requires \"components_url\""`
- **After**: `"External dependent variables require a 'components_url' field specifying where component data is stored"`

### 3. Validation Explanations
- **Before**: `"Unrecognized \"numeric_type\""`
- **After**: `"Unrecognized 'numeric_type' value '%s'. Expected values like 'float64', 'int32', 'complex128', etc."`

### 4. Memory Allocation Context
- **Before**: `"Failed to create name string"`
- **After**: `"Unable to process 'name' field value. This may indicate memory allocation issues"`

### 5. Component Processing Details
- **Before**: `"Component format doesn't match expected encoding"`
- **After**: `"Component format mismatch. Expected %s based on encoding '%s', but found incompatible data"`

### 6. Typed vs Untyped Format Context
- Added clear explanations for when fields are valid/invalid based on JSON format type
- Explained the difference between typed (`{"type": "DependentVariable", "value": {...}}`) and untyped formats

## Functions Improved

1. **`DependentVariableCreateFromJSON`** - Primary JSON parsing function
2. **`impl_DependentVariableCreate`** - Core creation function
3. **`DependentVariableCreateExternal`** - External variable creation
4. **`DependentVariableCreateFromDictionary`** - Dictionary-based creation

## Benefits for End Users

1. **Clear Problem Identification**: Users can quickly understand what went wrong
2. **Actionable Guidance**: Error messages suggest how to fix the problem
3. **Context Awareness**: Messages explain why certain fields are required or invalid
4. **Format Understanding**: Users learn about the different JSON format requirements
5. **Debugging Aid**: More specific error messages help developers trace issues faster

## Example Improved Error Messages

### Field Validation
```
"DependentVariable JSON parsing failed: Invalid 'quantity_type' value 'unknown'. Expected values like 'scalar', 'vector_3', 'matrix_3x3', 'pixel_rgba', etc."
```

### Type Constraints
```
"DependentVariable JSON parsing failed: Internal dependent variables cannot have a 'components_url' field. Remove this field or change type to 'external'"
```

### Data Processing
```
"DependentVariable JSON parsing failed: Cannot convert component numbers to specified numeric type 'float64': Invalid number format in array"
```

These improvements make the library more user-friendly and reduce debugging time for developers working with DependentVariable JSON data.
