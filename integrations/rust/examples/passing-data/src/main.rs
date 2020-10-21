// Import the wasmer runtime so we can use it
use wasmer_runtime::{error, imports, instantiate, Array, Func, WasmPtr};

// Our entry point to our application
fn main() -> error::Result<()> {
    // Let's get the .wasm file as bytes
    let wasm_bytes = include_bytes!("../../../../shared/rust/passing-data.wasm");

    // Now that we have the Wasm file as bytes, let's run it with the wasmer runtime
    // Our import object, that allows exposing functions to our Wasm module.
    // We're not importing anything, so make an empty import object.
    let import_object = imports! {};
    // Let's create an instance of Wasm module running in the wasmer-runtime
    let instance = instantiate(wasm_bytes, &import_object)?;

    // Lets get the context and memory of our Wasm Instance
    let wasm_instance_context = instance.context();
    let wasm_instance_memory = wasm_instance_context.memory(0);

    // Let's get the pointer to the buffer defined by the Wasm module in the Wasm memory.
    // We use the type system and the power of generics to get a function we can call
    // directly with a type signature of no arguments and returning a WasmPtr<u8, Array>
    let get_wasm_memory_buffer_pointer: Func<(), WasmPtr<u8, Array>> = instance
        .exports
        .get("get_wasm_memory_buffer_pointer")
        .expect("get_wasm_memory_buffer_pointer");
    let wasm_buffer_pointer = get_wasm_memory_buffer_pointer.call().unwrap();
    dbg!(wasm_buffer_pointer);
    // Let's write a string to the Wasm memory
    let original_string = "Did you know";
    println!("The original string is: {}", original_string);

    // We deref our WasmPtr to get a &[Cell<u8>]
    let memory_writer = wasm_buffer_pointer
        .deref(wasm_instance_memory, 0, original_string.len() as u32)
        .unwrap();
    for (i, b) in original_string.bytes().enumerate() {
        memory_writer[i].set(b);
    }

    // Let's call the exported function that concatenates a phrase to our string.
    let add_wasm_is_cool: Func<u32, u32> = instance
        .exports
        .get("add_wasm_is_cool")
        .expect("Wasm is cool export");
    let new_string_length = add_wasm_is_cool.call(original_string.len() as u32).unwrap();

    // Get our pointer again, since memory may have shifted around
    let new_wasm_buffer_pointer = get_wasm_memory_buffer_pointer.call().unwrap();
    // Read the string from that new pointer.
    let new_string = new_wasm_buffer_pointer
        .get_utf8_string(wasm_instance_memory, new_string_length)
        .unwrap();

    println!("The new string is: {}", new_string);
    // Asserting that the returned value from the function is our expected value.
    assert_eq!(new_string, "Did you know Wasm is cool!");

    // Log a success message
    println!("Success!");
    // Return OK since everything executed successfully!
    Ok(())
}
