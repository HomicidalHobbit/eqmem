use libc::c_void;
use std::marker::PhantomData;
use std::mem;
use std::{thread, time};

#[link(name = "eqmem", kind = "static")]
extern "C" {
    fn LocalAllocate(size: usize, tag: i32) -> *mut c_void;
    fn Deallocate(ptr: *mut c_void);
    fn SetLocalLogging(enable: bool);
}

fn main() {
    println!("Hello, world!");
    let t = thread::spawn(move || {
        println!("Second Thread");
        let secs = time::Duration::from_secs(5);
        thread::sleep(secs);
        unsafe {
            SetLocalLogging(true);
            let ptr = LocalAllocate(1024 * 1024 * 128, 0);
            Deallocate(ptr);
        }
    });
    unsafe {
        SetLocalLogging(true);
        let ptr = LocalAllocate(1024, 0);
        t.join().unwrap();
        Deallocate(ptr);
    }
    let _b = BinVec::<usize>::with_capacity(1024);
}

struct BinVec<T> {
    vec: Vec<T>,
}

impl<T> BinVec<T> {
    fn with_capacity(size: usize) -> BinVec<T> {
        unsafe {
            let ptr: *mut c_void = LocalAllocate(mem::size_of::<T>() * size, 0);
            BinVec {
                vec: Vec::<T>::from_raw_parts(ptr as *mut T, 0, 1),
            }
        }
    }
}

// This drop is going to do a double free!!!
// Since we're replacing the global_allocator. I don't think this matters we only need to make sure
// that the allocator is tracking the memory.
//
// This is here merely to prove that point and for reference.

impl<T> Drop for BinVec<T> {
    fn drop(&mut self) {
        unsafe {
            Deallocate(self.vec.as_mut_ptr() as _);
        }
    }
}
