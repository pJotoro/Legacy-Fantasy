# What I did right
*In comparison to other projects using Vulkan*, my code is very readable. That doesn't mean it *is* readable, exactly.

The problem is Vulkan's verbosity. It frightens most people. As a result, they immediately write their own abstraction layer over Vulkan, designed to simplify rendering as much as possible by automating things like synchronization and memory management. This is hilarious. The whole point of Vulkan was to *get rid of driver overhead* by *eliminating needless layers of abstraction*, not to *make your own*. Since most game companies are not qualified to write their own graphics driver, the result is that more and more game companies are switching over to Unity and Unreal. 

Now, I don't expect to fix the situation any time soon. However, I think I could be of much help to game companies (or game engine companies) who are using Vulkan (or another modern API).

Anyways, back to talking about my code: in order to cope with the complexity of Vulkan, I try to follow the advice of John Carmack, which you can read here: https://cbarrete.com/carmack.html. In particular, there is one paragraph that stands out:

> Using large comment blocks inside the major function to delimit the minor functions is a good idea for quick scanning, and often enclosing it in a bare braced section to scope the local variables and allow editor collapsing of the section is useful. I know there are some rules of thumb about not making functions larger than a page or two, but I specifically disagree with that now – if a lot of operations are supposed to happen in a sequential fashion, their code should follow sequentially.

This is especially true in Vulkan. You have to create an instance, so that way you can query the physical devices, so that way you can create a logical device, so that way, etc. Commands have to be recorded in a certain order; one wrongly placed pipeline barrier can cause a black screen and a million validation errors. With practice, these problems *can* be solved fairly quickly, but only if the code follows a sequential order.

You might say: isn't this a highly inefficient way of doing things? Honestly, I don't think so. I wrote the renderer for this prototype in a month and a half. Turning this into a whole game would be a lot of work, but as far as rendering goes, it's already mostly done. With more experience, I don't see why I wouldn't be able to do the same for a 3D game.

# What I would do differently
Building from source is an absolute pain. Let me show you what you have to do.

1. Clone the repository.
2. Add a folder called 'build'. This folder should be at the same level as 'code' and 'libraries'.
3. Inside 'build', add two folders, one called 'ninja' and the other called 'shaders'.
4. Inside 'build/ninja', add two folders, one called 'debug' and the other called 'release'.
5. Copy the contents of 'libraries/SDL3/bin' to both 'build/ninja/debug' and 'build/ninja/release'.
6. If you haven't already, please install CMake and Ninja. This can be "easily" done with winget or similar.
7. In the command prompt, go back to the outermost directory, where 'code', 'build', and so on are.
8. Run the following command: 'cmake --preset=debug'.
9. Run the following command: 'cmake --preset=release'.
10. Now, in your favorite IDE or code editor, you can "easily" call 'ninja' in either the build or release folder.
11. Finally, remember to call build_shaders_debug.bat or build_shaders_release.bat depending on if you want to do a debug or release build. The resulting SPIR-V files are put in 'build/shaders'.

Notice all the times where I said "easily." The reality is, it's not always that easy. You never know what another person's starting point is. Not everyone knows how to use a command prompt, or how to mess with build settings in their favorite editor. It is always better to make these things as automatic as possible so that way, few if any problems can happen. Were I to do this again, I would automatically install all the libraries using CMake and vcpkg, and include a Visual Studio project configured to build with CMake. I would also make the building of shaders happen either as a custom build command in CMake, or at runtime by the renderer itself.

Furthermore, I don't like that all the dependencies are included in the repository. There is a certain segment of programmers who think that is the way, but I disagree. It just adds unnecessary bloat to the repository.

It's also good to ask whether I even needed some of these libraries. I have enough experience using the Windows API that I don't really need SDL on Windows. Furthermore, the only parts of cglm I'm really using are just simple vector math, which I could have easily implemented myself. However, I didn't want to spend an eternity working on this, and I wasn't sure at the time what would take long versus what wouldn't.