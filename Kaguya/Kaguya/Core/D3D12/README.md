# D3D12

D3D12 wrappers is inspired by Unreal Engine's D3D12RHI, the main abstractions took from D3D12RHI is CommandQueue/CommandList/CommandContext, I really liked the way they did Command object abstractions, it allows for conveninent command recording and command submission.

Memory allocators:

- Linear allocator, it uses a paging system, by default it allocates 2MB memory per page, page is tracked via fence values to ensure the data is not overriden when the GPU access it

Some allocators that I want to implement/implementing:

- Buddy system, The technique works by maintaining a binary tree that tracks the splitting of blocks of memory. When an allocation is requested, larger blocks are continually split until either further splitting would result in a block that is too small to satisfy the request or the smallest size block is reached

- Seglist (Segregated list) allocator, where it provides an array of free lists, where each array contains blocks of the same size (power of 2)
