# Legacy Fantasy
This is a prototype of a 2D platformer game rendered using the Vulkan API. [Here is a demo of it.](https://www.youtube.com/watch?v=J81yIJgHqP0)

## What I did
Everything except for the art and libraries I used. That is: I wrote all the game code in C; the shaders in GLSL; and the makeshift build system in a mix of CMake, batch files, and a Sublime Text project file. Although I didn't make any of the art, I did have to manually add hitboxes to the player and boar sprites; I also had to add an origin to each sprite in order for them to render in the right place.

## What I didn't do
The art can be found here: https://anokolisa.itch.io/sidescroller-pixelart-sprites-asset-pack-forest-16x16.

To be clear, I have no association to the person who made this art. The fact that I also named this project "Legacy Fantasy" was probably a mistake. I just wasn't sure what else to call it.

Here are links to all the libraries I used:

#### cglm
https://github.com/recp/cglm

#### cJson 
https://github.com/DaveGamble/cJSON

#### SDL3
For this I used an automatic build instead of the official repository. The automatic build I used can be found here: https://github.com/mmozeiko/build-sdl3.
The official repository can be found here: https://github.com/libsdl-org/SDL

#### spall
This is not a library exactly. It's a profiler, but it comes with a library which you use to record a profile, which you then open with Spall afterwards. You can buy it here: https://gravitymoth.itch.io/spall

#### vk_video, Volk, vulkan
These came with the Vulkan SDK, which I downloaded here: https://vulkan.lunarg.com/

#### infl.h
This one is a little bit complicated. The original repository can be found here: https://github.com/vurtun/lib/blob/master/sinfl.h

However, I did not use that version because it apparently had bugs. Those bugs have been fixed since then, but in two separate repositories:

https://github.com/raysan5/raylib/blob/master/src/external/sinfl.h
https://github.com/EpicGamesExt/raddebugger/blob/master/src/third_party/sinfl/sinfl.h

The fixes made in these two forks are not the same fixes, but both of them turned out to be necessary in order for my code to work.

#### raddbg_markup.h
This is not a library exactly, but a set of macros to be used with Raddebugger, which can be found here: https://github.com/EpicGamesExt/raddebugger/

#### xxhash.h
https://github.com/Cyan4973/xxHash/blob/dev/xxhash.h

## Rationale
Why put yourself through the insanity of making a game without a game engine? Why not just use Unity or Godot? Believe it or not, I'm no elitist when it comes to making games. Allow me to explain.

I started out with Scratch when I was eleven years old. By the time I was thirteen, I was so sick of Scratch that I intentionally got myself banned. After that, I tried several different game engines including Unity, Unreal, GameMaker and Construct. In the end, I went with GameMaker because I felt it was the easiest to learn while still having the features I wanted. 

By the time I was fifteen, I was already sick of GameMaker; that is, I went full circle. Just like before, I tried messing around with different game engines including Unity, Unreal and Godot. In the end, I went with none of them because I found a series called Handmade Hero which teaches you how to make a complete game from scratch without any game engine, libraries, frameworks or whatever else; while watching that series, I realized that computer programming as a whole, not just game programming, is actually extremely interesting. Consequently, over the next three years I learned as much as I possibly could about how everything works under the hood.

And then I went to college. Over all those years, I assumed that what I had learned was merely computer *programming*, and that only in college would I finally learn computer *science*; funnily enough, the very first thing I noticed in college was that I knew more than my professors.

So what? Did you drop out so you could make this game? Actually, no. I kept going, and ended up getting an Associate's degree (for any non-Americans out there, it's basically half a Bachelor's degree). In the end, it was well worth it for one simple reason: 90% of my time was spent studying math, which was the one thing that I still hadn't learned while I was in high school (fun fact: I learned Vulkan before I learned algebra). I now know math up to linear algebra and am currently self-teaching real analysis.

After two years of college, I realized that I had no reason to go anymore. I already knew all the math that you would normally learn in a computer science degree, and a lot more about programming. Given that, I had what you might call *equivalent experience*. I figured: why not make a project to show off what I've learned over all these years? And there's your answer.

## Challenges I faced along the way
By far, the biggest one was actually in being clear about which coordinate system I was using, AKA which *space* I was in. The game has three types of space: object space, level space and tile space. I probably lost about a month of work from bugs caused by me mixing up tile space with level space. Tiles would end up rendering in the wrong position, or the player would not collide with the level properly.

Vulkan also caused a lot of problems for me, but not for the reason you might think. Vulkan has gotten a bad wrap for being extremely verbose. The reality is that as long as you don't prematurely abstract over it, it's not that bad. The real problems came from misunderstandings about what the *point* of certain things in the Vulkan API actually is, especially image layout transitions. I erroneously thought that every image automatically gets reset to VK_IMAGE_LAYOUT_UNDEFINED after calling vkQueuePresentKHR; in reality, once an image is transitioned to an image layout, it stays in that image layout unless it is transitioned again. Now that the prototype is done, I understand *how* image layout transitions work, but not *why* they exist. I guess I just have to make friends with someone who works at AMD or Nvidia?

This last one might surprise you. It was very often the case that individually, there was nothing wrong with the gameplay, physics, rendering or animation code; instead, one of those four would make faulty assumptions about how the other three systems worked. For example, there was a bug where the player would glitch through walls while walking left; you'd think the problem was in the physics code, but the real problem was that the player hitbox wasn't getting flipped correctly because the origin of the player sprite was set at the top-left, not at the player's feet. I can only imagine how complicated this might get at a large company where each team might *think* they are doing a good job, until everything breaks.