#include <stdio.h>
#include "dist/wasmer-build/src/wasmer-runtime-c-api/lib/runtime-c-api/wasmer.h"
#include <assert.h>
#include <string.h>

// Use the last_error API to retrieve error messages
void print_wasmer_error()
{
  int error_len = wasmer_last_error_length();
  printf("Error len: `%d`\n", error_len);
  char *error_str = malloc(error_len);
  wasmer_last_error_message(error_str, error_len);
  printf("Error str: `%s`\n", error_str);
}

wasmer_memory_t *create_wasmer_memory() {
  // Create our initial size of the memory 
  wasmer_memory_t *memory = NULL;
  // Create our maximum memory size.
  // .has_some represents wether or not the memory has a maximum size
  // .some is the value of the maxiumum size
  wasmer_limit_option_t max = { .has_some = true,
                                .some = 256 };
  // Create our memory descriptor, to set our minimum and maximum memory size
  // .min is the minimum size of the memory
  // .max is the maximuum size of the memory
  wasmer_limits_t descriptor = { .min = 256,
                                 .max = max };

  // Create our memory instance, using our memory and descriptor,
  wasmer_result_t memory_result = wasmer_memory_new(&memory, descriptor);
  // Ensure the memory was instantiated successfully.
  if (memory_result != WASMER_OK)
  {
    print_wasmer_error();
  }

  return memory;
}

wasmer_module_t *create_wasmer_module() {
  // Read the wasm file bytes
  // TODO: Check if file is NULL
  FILE *file = fopen("example-wasienv-wasm/strings-wasm-is-cool/strings-wasm-is-cool.wasm", "r");
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  uint8_t *bytes = malloc(len);
  fseek(file, 0, SEEK_SET);
  fread(bytes, 1, len, file);
  fclose(file);

  wasmer_module_t *module = NULL;
  wasmer_result_t compile_result = wasmer_compile(&module, bytes, len);

  // Print our the result of our compilation,
  // Ensure the compilation was successful.
  printf("Compile result:  %d\n", compile_result);
  if (compile_result != WASMER_OK)
  {
    print_wasmer_error();
  }
  // Assert the wasm instantion completed
  assert(compile_result == WASMER_OK);


  free(bytes);

  return module;
}

wasmer_import_object_t *create_import_object(wasmer_memory_t *memory) {
  // Create module name for our imports

  // Create a UTF-8 string as bytes for our module name. 
  // And, place the string into the wasmer_byte_array type so it can be used by our guest wasm instance.
  const char *module_name = "env";
  wasmer_byte_array module_name_bytes = { .bytes = (const uint8_t *) module_name,
    .bytes_len = strlen(module_name) };

  // Define a memory import

  // Create a UTF-8 string as bytes for our module name. 
  // And, place the string into the wasmer_byte_array type so it can be used by our guest wasm instance.
  const char *import_memory_name = "memory";
  wasmer_byte_array import_memory_name_bytes = { .bytes = (const uint8_t *) import_memory_name,
                                                 .bytes_len = strlen(import_memory_name) };

  // Create our memory import object, from our passed memory,
  // that will be used as shared wasm memory between the host (this application),
  // and the guest wasm module.
  // The .module_name is the key of the importObject that this memory is associated with.
  // The .import_name is the key of the module that is within the importObject
  // The .tag is the type of import being added to the import object
  wasmer_import_t memory_import = { .module_name = module_name_bytes,
                                    .import_name = import_memory_name_bytes,
                                    .tag = WASM_MEMORY };

  // Set the memory to our import object
  memory_import.value.memory = memory;
  // Define a global import
  const char *import_global_name = "__memory_base";
  wasmer_byte_array import_global_name_bytes = { .bytes = (const uint8_t *) import_global_name,
                                                 .bytes_len = strlen(import_global_name) };
  wasmer_import_t global_import = { .module_name = module_name_bytes,
                                    .import_name = import_global_name_bytes,
                                    .tag = WASM_GLOBAL };

  wasmer_value_t val = { .tag = WASM_I32,
                           .value.I32 = 1024 };
  wasmer_global_t *global = wasmer_global_new(val, false);
  global_import.value.global = global;

  // Define an array containing our imports
  wasmer_import_t imports[] = {global_import, memory_import};

  
  wasmer_import_object_t *import_object = wasmer_import_object_new();
  wasmer_result_t extend_result = wasmer_import_object_extend(import_object, imports, 2);

  if (extend_result != WASMER_OK)
  {
    print_wasmer_error();
  }

  // Assert the wasm instantion completed
  assert(extend_result == WASMER_OK);

  return import_object;
}

// Function to create our Wasmer Instance
// And return it's memory
wasmer_instance_t *create_wasmer_instance(wasmer_module_t *module, wasmer_import_object_t *import_object) {
  // Instantiate a WebAssembly Instance from a module and import object
  wasmer_instance_t *instance = NULL;
  wasmer_result_t instantiate_result = wasmer_module_import_instantiate(&instance,
                                                                        module,
                                                                        import_object);

  // Print our the result of our compilation,
  // Ensure the compilation was successful.
  printf("Instantiation result:  %d\n", instantiate_result);
  if (instantiate_result != WASMER_OK)
  {
    print_wasmer_error();
  }

  // Assert the wasm instantion completed
  assert(instantiate_result == WASMER_OK);

  return instance;
}

int call_wasm_function_and_return_i32(wasmer_instance_t *instance, char* functionName, wasmer_value_t params[], int num_params) {
  // Define our results. Results are created with { 0 } to avoid null issues,
  // And will be filled with the proper result after calling the guest wasm function.
  wasmer_value_t result_one = { 0 };
  wasmer_value_t results[] = {result_one};


  // Call the wasm function
  wasmer_result_t call_result = wasmer_instance_call(
      instance, // Our Wasm Instance
      functionName, // the name of the exported function we want to call on the guest wasm module
      params, // Our array of parameters
      num_params, // The number of parameters
      results, // Our array of results
      1 // The number of results
      );

  // Get our response, we know the function is an i32, thus we assign the value to an int
  int response_tag = results[0].tag;
  int response_value = results[0].value.I32; 

  return response_value;
}

uint8_t *get_pointer_to_memory(wasmer_instance_t *instance) {
  const wasmer_instance_context_t *ctx = wasmer_instance_context_get(instance);
  const wasmer_memory_t *memory = wasmer_instance_context_memory(ctx, 0);

  return wasmer_memory_data(memory);
}

int main() {

  // Initialize our Wasmer Memory and Instance
  wasmer_memory_t *memory = create_wasmer_memory();
  uint32_t memoryLength = wasmer_memory_data_length(memory);
  wasmer_module_t *module = create_wasmer_module();
  wasmer_import_object_t *import_object = create_import_object(memory);
  wasmer_instance_t *instance = create_wasmer_instance(module, import_object);

  // Let's get the pointer to the buffer exposed by our Guest Wasm Module
  wasmer_value_t getBufferPointerParams[] = { 0 };
  int bufferPointer = call_wasm_function_and_return_i32(instance, "getBufferPointer", getBufferPointerParams, 0);

  printf("Wasm buffer pointer: %d\n", bufferPointer);

  // Write our initial string into the buffer
  // Get a pointer to the memory Data
  uint8_t *memoryData = get_pointer_to_memory(instance);
  printf("memory pointer %p", memoryData);
  char *memoryBytes = memoryData;
  if (!memoryBytes) {
    // TODO: Handle the Null pointer
  }
  // Offset set it to where the buffer is in the guest wasm
  printf("memoryLength: %d\n", memoryLength);
  printf("memoryBytes: %p\n", memoryBytes);
  printf("Wasm value at buffer pointer, *(memoryBytes + bufferPointer): %d\n", *(memoryBytes + bufferPointer));

  for(int i = -10; i < 10; i++) {
    printf("Surrounding Wasm value at buffer pointer, i: %d, *(memoryBytes + bufferPointer): %d\n", i, *(memoryBytes + bufferPointer + i));
  }


  // Write the string bytes to memory
  char originalString[13] = "Hello there,";
  int originalStringLength = sizeof(originalString) / sizeof(originalString[0]);
  printf("originalStringLength: %d\n", originalStringLength);
  for (int i = 0; i < originalStringLength; i++) {
    *(memoryBytes + bufferPointer + i) = originalString[i];
  }

  // Call the exported "addWasmIsCool" function of our instance
  wasmer_value_t param_original_string_length = { .tag = WASM_I32, .value.I32 = originalStringLength };
  wasmer_value_t addWasmIsCoolParams[] = { param_original_string_length };
  int newStringLength = call_wasm_function_and_return_i32(instance, "addWasmIsCool", addWasmIsCoolParams, 1);
  printf("newStringLength: %d\n", newStringLength);

  // Fetch out the new string
  char newString[100];

  memoryData = get_pointer_to_memory(instance);
  for (int i = 0; i < newStringLength; i++) {
    char charInBuffer = *(memoryBytes + bufferPointer + i);
    printf("Char: %c\n", charInBuffer);
    newString[i] = charInBuffer;
  }

  printf("new string %s\n", newString);

  // TODO: Print and assert the new string
  
  return 0;
}
