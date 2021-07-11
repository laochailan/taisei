Complex Numbers in Taisei
=========================

.. contents::

Introduction
''''''''''''

Taisei uses `complex numbers <https://en.wikipedia.org/wiki/Complex_number>`__
in its game code for a number of different functions, such as:

* Player and enemy position and movement
* Danmaku patterns and bullet movement
* Special effects and particles
* Spell card backgrounds for bosses

As you can probably see, it's important to have a firm grasp of complex numbers
in order to make heads or tails of what the game is doing if you want to
develop for it.

Complex numbers shouldn't be scary. If you have a baseline of middle school
mathematics, and certainly if you're familiar with the concept of "variables"
in programming, it should only take you a couple of hours to get used to
thinking in these terms, and using them effectively in the code.

There are many different places you can learn about complex numbers, but we've
found these two YouTube creators do a better job than we could of explaining
the core concepts behind complex numbers.

* `Imaginary Numbers are Real <https://www.youtube.com/watch?v=T647CGsuOVU&list=PLiaHhY2iBX9g6KIvZ_703G3KJXapKkNaF>`__
  by Welch Labs - short and punchy video series
* `Complex Number Fundamentals <https://www.youtube.com/watch?v=5PcpBw5Hbwo>`__
  by 3Blue1Brown - a longer lecture-style format

In-Game Examples
''''''''''''''''

Simple Movement
^^^^^^^^^^^^^^^

Lets take a few concrete examples within Taisei's code. For let's look at a
piece of movement code for a fairy:

.. code-block:: c

   enemy->move = move_towards(VIEWPORT_W/2.0 + 200.0 * I, 0.035);

The important piece here, for our purposes, is:

.. code-block:: c

   VIEWPORT_W/2.0 + 200.0 * I

What this is doing is specifying a position on the X-axis (``VIEWPORT_W/2.0``,
which in Taisei means ``480 / 2.0`` or `240.0`), and then specifying a position
on the "imaginary" (aka "lateral") axis (``200.0 * I``). The ``I`` here is the
same ``i`` described in the videos and the above explanation.

So what we're really looking at here is:

.. code-block:: c

   240.0 (real) + 200.0i (imaginary)

Or "move 240 units on the X-axis, and then 200 units on the Y-axis (called the
'imaginary axis' in complex vectors)."

This is what's called a **Cartesian Coordinate.** What the function
``move_towards`` then does is make the enemy sprite/object move towards that
point on the X/Y axis at a certain rate (defined by ``0.035``).

Simple Danmaku
^^^^^^^^^^^^^^

Of course, there'd be no point in using complex numbers over a simpler X/Y
system if it didn't provide significant advantages downstream. So let's look at
a more complicated danmaku pattern to see why complex numbers are more
effective for this role.

.. code-block:: c

   cmplx aim = cnormalize(global.plr.pos - enemy->pos);

This ``aim`` variable could be passed to a ``move_towards`` function attached
to a ``PROJECTILE`` object. The effect here would be the bullets moving
directly towards the player in a straight line, even if the player moves around
the screen, following the player as they move around in real-time (assuming)
more than one bullet is fired.

Let's look at the argument inside ``cnormalize`` first, ``global.plr.pos -
e->pos``.  Both ``global.plr.pos`` and ``e->pos`` are *complex numbers*, in
that they have both *real* and *imaginary* parts. Much like the example in
``Simple Movement``, they represent a place on the X/Y grid.

In the format of ``[X,Y]``, let's say that ``global.plr.pos`` is ``[-1, 6]``,
and that ``enemy->pos`` is ``[6, 3]``.

.. image:: images/math-01.png
   :width: 300pt

When you subtract ``[6, 3]`` (enemy position) from ``[-1, 6]`` (player
position), you end up with ``[-7, 3]``, as seen here with ``plr->pos``.

.. image:: images/math-02.png
   :width: 300pt

This also conveniently lets the enemy position ``enemy->pos`` become the new
"origin," or ``[0, 0]``. This is useful because it means that we can more
easily determine what angle the danmaku need to travel in to travel towards the
player.

Since we don't really care about the distance, as we're looking for an angle
or direction instead, ``cnormalize()`` in the original code mostly ignores
that so that we get the complex number between ``[0, 0]`` and ``[-7, 3]``.
You can see the formula used for converting a *cartesean coordinate* to a
*polar coordinate* `here
<https://www.engineeringtoolbox.com/converting-cartesian-polar-coordinates-d_1347.html>`__,
but for the sake of brevity, the angle between the two, assuming **0°** is
"positive" on the X-axis, is **157°**, which you can see from the orange line
rotating counter-clockwise to about **157°** on the second image.

This could still be done, technically, using traditional vectors. However,
there are still significant advantages to doing it this way. Let's consider how
we might use this new ``aim`` variable later on, say in a ``PROJECTILE`` block
for a danmaku bullet:

.. code-block:: c

   // aim directly at the player
   cmplx aim = cnormalize(global.plr.pos - enemy->pos);

   // a bit of randomization
   cmplx offset = cdir(M_PI/180 * rng_sreal());

   // later, inside a PROJECTILE() block...
   .move = move_asymptotic_simple(aim * offset, 5),

The important piece here is the ``aim * offset`` inside the ``move()`` block.
Being able to multiply complex numbers by each other means "procedurally"
generating danmaku patterns becomes much easier. Multiplying two complex
numbers together like this means their **polar coordinates** are multiplied,
and in the case of something like ``cdir(M_PI/180 * rng_sreal())``, you can add
"predictable randomization" to your patterns without having to wrangle with
cumbersome vector matricies. You can modify the original direction of "shoot
directly at the player" of ``aim`` with an additional ``offset`` angle.

Additionally, the C programming language has a very robust support for handling
complex numbers, whereas the support for things like vector matricies isn't as
available or pleasant to use.

With a bit of extra initial setup, you end up with code that's much easier to
maintain and understand.

Further Reading
'''''''''''''''

While Taisei doesn't use them, a more advanced form of these complex numbers,
called `quaternions <https://en.wikipedia.org/wiki/Quaternion>`__, are used in
3D games and programming very extensively in most major 3D engines. Hopefully
this helps solidify that complex numbers aren't some special thing that Taisei
invented or repurposed, and are applicable in many other scenarios when it
comes to game programming and design.
