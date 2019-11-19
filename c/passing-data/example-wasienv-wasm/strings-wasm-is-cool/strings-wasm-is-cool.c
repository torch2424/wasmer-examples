// Define a fixed size bbyte array
char byteBuffer[1000];

char* getBufferPointer() {
  return byteBuffer;
}

// Function that will add the string "Wasm is cool" to the pointer for the original string
int addWasmIsCool(int originalStringLength) {
  char wasmIsCool[15] = " Wasm is cool!";
  int wasmIsCoolLength = sizeof(wasmIsCool) / sizeof(wasmIsCool[0]);

  for (int i = 0; i < wasmIsCoolLength; i++) {
    byteBuffer[originalStringLength + i] = wasmIsCool[i];
  }

  // Return the new length of the string
  return originalStringLength + wasmIsCoolLength;
}
