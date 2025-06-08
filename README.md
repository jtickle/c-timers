This is just me messing with C and trying to see how performant stuff is.

You can just run `make` to build everything.

`sizes` shows some type sizes on your platform.

`timefork` times sending a single byte from a parent to a child.

`timepipe` was a lot of fun. It reads a gigabyte of random data (this
operation takes an interesting amount of time) and then uses multiple
buffer lengths to pipe that data from a parent to a child. There are
some debug printf's that are commented out that were instrumental in
figuring out why things weren't working during development.

`timexalloc` compares the time it takes to malloc vs calloc. Because
calloc results in memory that's initialized to zero, I expected that
it would take longer to perform than malloc due to having to
initialize the memory. It did take longer but not as much as I thought
it would. Note that it also tries a malloc + memcpy and malloc +
manual initalization; both of which perform absolutely atrociously
when compared to calloc. I have no idea how calloc can be so fast.
It is interesting though that calloc's worst performance is at 4mb
isn't it?
