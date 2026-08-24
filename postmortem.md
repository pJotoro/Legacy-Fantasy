# What I did right
*In comparison to other projects using Vulkan*, my code is very readable. That doesn't mean it *is* readable, exactly.

The problem is Vulkan's verbosity. It frightens most people. As a result, they immediately write their own abstraction layer over Vulkan, designed to simplify rendering as much as possible by automating things like synchronization and memory management. This is hilarious. The whole point of Vulkan was to *get rid of driver overhead* by *eliminating needless layers of abstraction*, not to *make your own*. Since most game companies are not qualified to write their own graphics driver, the result is that more and more game companies are switching over to Unity and Unreal.

Now, I don't expect to fix the situation any time soon. However, I think I could be of much help to game companies (or game engine companies) who are using Vulkan (or another modern API).

Anyways, back to talking about my code: in order to cope with the complexity of Vulkan, I try to follow the advice of John Carmack, which you can read here: https://cbarrete.com/carmack.html. In particular, there is one paragraph that stands out:

> Using large comment blocks inside the major function to delimit the minor functions is a good idea for quick scanning, and often enclosing it in a bare braced section to scope the local variables and allow editor collapsing of the section is useful. I know there are some rules of thumb about not making functions larger than a page or two, but I specifically disagree with that now – if a lot of operations are supposed to happen in a sequential fashion, their code should follow sequentially.

This is especially true in Vulkan. You have to create an instance, so that way you can query the physical devices, so that way you can create a logical device, so that way, etc. Commands have to be recorded in a certain order; one wrongly placed pipeline barrier can cause a black screen and a million validation errors. With practice, these problems *can* be solved fairly quickly, but only if the code follows a sequential order.

You might say: isn't this a highly inefficient way of doing things? Honestly, I don't think so. I wrote the renderer for this prototype in a month and a half. Turning this into a whole game would be a lot of work, but as far as rendering goes, it's already mostly done. If I had professional experience as a graphics programmer, then I don't see why I wouldn't be able to do the same for a 3D game.

# What I would do differently
I would automatically install all the libraries using CMake and vcpkg, and include a Visual Studio project configured to build with CMake. The way it is right now, I would have to list out every single library you need to install and exactly how to put it in the libraries folder. Actually, let's do that right now.

In order to build from source, "simply" follow these steps:
1. Add a folder called 'libraries'. This folder should be at the same level as 'code.'
2. Enter the command prompt inside that folder.
3. If you haven't already, you should install git. This can be "easily" done with winget or similar.
4. Run the following command: 'git clone https://github.com/recp/cglm'.
5. Run the following command: 'git clone https://github.com/DaveGamble/cJSON'.
6. Run the following command: 'git clone https://github.com/Jack-Punter/spall'.
7. Run the following command: 'git clone https://github.com/Cyan4973/xxHash'.
8. Download and unzip the latest release of 'https://github.com/mmozeiko/build-sdl3' (specifically SDL3-x64-XXXX-XX-XX.zip). Copy the folder 'SDL3-x64/include/SDL3' to 'libraries'. Copy the folder 'SDL3-x64/lib' to 'libraries/SDL3'.
9. Add a folder called 'build'. This folder should be at the same level as 'code' and 'libraries'.
10. Inside 'build', add two folders, one called 'ninja' and the other called 'shaders'.
11. Inside 'build/ninja', add two folders, one called 'debug' and the other called 'release'.
12. Copy the contents of 'SDL3-x64/bin' to both 'build/ninja/debug' and 'build/ninja/release'.
13. If you haven't already, please install CMake and Ninja. This can be "easily" done with winget or similar.
14. In the command prompt, go back to the outermost directory, that is where 'code', 'build', and so on are.
15. Run the following command: 'cmake --preset=debug'.
16. Run the following command: 'cmake --preset=release'.
17. Now, in your favorite IDE or code editor, you can "easily" call 'ninja' in either the build or release folder.

Notice all the times where I said "easily" or "simply." The reality is, it's not always so easy. You never know what another person's starting point is. Not everyone knows how to use a command prompt or a package manager, or how to mess with build settings in their favorite editor. It is always better to make these things as automatic as possible so that way, few if any problems can happen.

Furthermore, whenever a project requires certain dependencies, the versions of those dependencies should always be specified by the build system. That way, it is always guaranteed that everyone uses the same versions. If an update to a particular dependency is desired, then that requires a change to the build system, which then gets saved in the version control system's history.