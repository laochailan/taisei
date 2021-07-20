/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

/*
 * Stage 1
 */

DIALOG_TASK(marisa, Stage1PreBoss) {
	DIALOG_BEGIN(Stage1PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(cirno);
	HIDE(cirno);

	FACE(marisa, puzzled);
	MSG(marisa, "Aw, snow again? I just put away my winter coat.");
	FACE(marisa, sweat_smile);
	MSG(marisa, "…even though it’s half-way through summer already.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(cirno, 180, "Nice, right?");

	SHOW(cirno);
	FACE(cirno, normal);
	MSG(cirno, "It’s my snow. I did it!");

	FACE(marisa, normal);
	MSG(marisa, "Mind if I borrow some money for the drycleanin’ bill?");
	FACE(marisa, sweat_smile);
	MSG(marisa, "I was gonna get it done in the fall, but now…");

	TITLE(cirno, "Cirno", "Thermodynamic Ice Fairy");
	MSG(cirno, "No, but I can lend you some ice cream!");
	EVENT(music_changes);
	FACE(cirno, angry);
	MSG(cirno, "After I turn YOU into ice cream!");

	FACE(marisa, smug);
	MSG(marisa, "I prefer shaved ice, myself.");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(cirno);
	VARIANT(cirno, defeated);
	FACE(cirno, defeated);

	FACE(marisa, smug);
	MSG(marisa, "Why don’t you go play with your new friend, the one from Hell?");
	MSG(marisa, "Maybe then it’d be more of a fair fight.");

	MSG(cirno, "Ouch…");
	MSG(cirno, "I-I mean, last time she lost! And next time, you will too!");

	MSG(marisa, "Sure, sure.");

	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(marisa, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(hina);
	HIDE(hina);
	FACE(marisa, unamused);
	MSG(marisa, "Geez, this place is kinda sad now.");
	MSG(marisa, "What could be causin’ the yōkai to get all weird, anyways?");

	EVENT(boss_appears);
	WAIT(60);
	SHOW(hina);
	FACE(hina, normal);
	MSG(hina, "Why hello, Ms. Kirisame. It’s been quite a while.");
	MSG(hina, "You seem to be as reckless as always. I recommend turning back.");

	FACE(marisa, happy);
	MSG(marisa, "As always, my reputation as an Incident Solver is at stake here, so I’ll be ignorin’ that advice.");
	FACE(marisa, normal);
	MSG(marisa, "Thanks, though. See ya.");

	TITLE(hina, "Kagiyama Hina", "Gyroscopic Pestilence God");
	FACE(hina, serious);
	MSG(hina, "Oh my… you’re quite the rebellious one, aren’t you?");
	MSG(hina, "Such confidence, ignoring my warning like that.");

	FACE(marisa, happy);
	MSG(marisa, "I’ll take that as a compliment.");

	EVENT(music_changes);
	MSG(hina, "And do you know what happens to naughty girls who don’t listen to their elders?");
	MSG(hina, "They get showered in misfortune for years!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(hina);
	VARIANT(hina, defeated);
	FACE(hina, defeated);
	FACE(marisa, unamused);
	MSG(marisa, "Y’know, you’re way more snippy than usual today, Hina.");
	MSG(marisa, "It’s kinda annoyin’, honestly.");

	MSG(hina, "H-how mean…");
	MSG(hina, "Don’t you see I was just trying to keep you safe…?");

	MSG(marisa, "Oi, don’t get all motherly on me now.");
	MSG(marisa, "I’ve had enough of that for a lifetime.");

	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(marisa, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(wriggle);
	HIDE(wriggle);
	FACE(marisa, normal);
	MSG(marisa, "Everyone’s more reckless today. What gives?");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(wriggle, 180, "Wait ‘til you see their leader!");
	SHOW(wriggle);

	MSG(marisa, "Yeah, I’m gonna have to ask their leader a bunch‘a questions for sure.");

	MSG(wriggle, "Feel free to go ahead. I’m listening.");

	MSG(marisa, "Oh, you’re her messenger?");
	FACE(marisa, happy);
	MSG(marisa, "Mind passin’ somethin’ along?");

	TITLE(wriggle, "Wriggle Nightbug", "Insect Rights Activist");
	FACE(wriggle, outraged);
	MSG(wriggle, "Um, OBVIOUSLY I’m not the messenger!");
	FACE(wriggle, proud);
	MSG(wriggle, "Yeah, I’m the great leader of Insectkind!");

	FACE(marisa, smug);
	MSG(marisa, "Great leader? What, did the insects unionize or somethin’?");

	FACE(wriggle, outraged);
	MSG(wriggle, "No! I told you, I’m the leader! The mastermind!");
	FACE(wriggle, calm);
	MSG(wriggle, "So, what’s your question?");

	FACE(marisa, normal);
	MSG(marisa, "Hmm, guess I forgot.");
	MSG(marisa, "What’s yer deal, anyways?");

	MSG(wriggle, "We insects have three demands:");
	MSG(wriggle, "First: say nay to insect spray!");
	MSG(wriggle, "Second: swat the swatters!");
	MSG(wriggle, "Third: the right to bite!");
	MSG(wriggle, "Fourth—");

	FACE(marisa, unamused);
	MSG(marisa, "Okay okay, this is great ‘n all, but I’ve honestly got more important stuff to do.");
	MSG(marisa, "… and ya don’t got a clue about what’s goin’ on, do ya?");

	EVENT(music_changes);
	FACE(wriggle, proud);
	MSG(wriggle, "Maybe I know more than you think.");
	FACE(wriggle, outraged);
	MSG(wriggle, "But it’s NOTHING I’d tell an usurper of Insectkind’s glory!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(wriggle);
	VARIANT(wriggle, defeated);
	FACE(wriggle, defeated);

	FACE(marisa, smug);
	MSG(marisa, "Tougher than usual, but not tough enough.");
	MSG(marisa, "Where’d ya pick up those new moves, anyway?");

	MSG(wriggle, "Aw! It was um… the great vision of our glorious Insect past!");
	MSG(wriggle, "It was in the age of Car—… Carb—… the age of Carbs!");

	FACE(marisa, puzzled);
	MSG(marisa, "‘Glorious insect past’? ‘Carbs’?");
	FACE(marisa, normal);
	MSG(marisa, "Oh, ya must mean the Carboniferous Period. With the giant bugs ‘n all that.");
	MSG(marisa, "I remember that doctor in the bamboo forest goin’ off about it once.");
	MSG(marisa, "Only time I’d seen her so excited…");

	MSG(wriggle, "Yeah, that!");
	MSG(wriggle, "Do you want to be an ally in our glorious conquest…?");

	FACE(marisa, smug);
	MSG(marisa, "Maybe later.");
	FACE(marisa, happy);
	MSG(marisa, "Probably never.");

	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(marisa, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(kurumi);
	HIDE(kurumi);

	FACE(marisa, surprised);
	MSG(marisa, "Oh boy, a mysterious mansion!");
	FACE(marisa, smug);
	MSG(marisa, "… how generic can you get?");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(kurumi, 180, "And to top it off, a western witch!");
	SHOW(kurumi);
	MSG(kurumi, "Seen it a thousand times already. It’s such a boring motif.");

	FACE(marisa, happy);
	MSG(marisa, "Right? But I make up for it with my devilish charm.");

	FACE(kurumi, dissatisfied);
	MSG(kurumi, "But that theme is so overdone…");
	FACE(kurumi, normal);
	MSG(kurumi, "What’s next? A maid?");
	FACE(marisa, normal);
	MSG(kurumi, "A little girl with almost no dialogue who becomes a fan-favourite anyways?");
	MSG(kurumi, "How about one that’ll have her personality massacred by the fans?");

	FACE(marisa, smug);
	MSG(marisa, "Oh, I wouldn’t know anythin’ about that.");

	MSG(kurumi, "Maybe I’ve been reading too much manga that those tengu churn out.");

	FACE(marisa, puzzled);
	MSG(marisa, "The tengu write manga now?");
	FACE(marisa, sweat_smile);
	MSG(marisa, "Writin’ pure ficiton is more honest for them, I guess.");

	TITLE(kurumi, "Kurumi", "High-Society Phlebotomist");
	FACE(kurumi, normal);
	MSG(kurumi, "Yeah, duh?");
	MSG(kurumi, "Of course, most of it is garbage that only weirdos like!");
	FACE(kurumi, dissatisfied);
	MSG(kurumi, "And don’t EVEN get me started on certain character designs! So gross!");

	FACE(marisa, normal);
	MSG(marisa, "Maybe they oughta hire someone with some fashion sense.");

	FACE(kurumi, normal);
	MSG(kurumi, "That’d be me, then!");
	MSG(kurumi, "My ideas would surely make a splash, if they’d recognize my talent!");

	FACE(marisa, unamused);
	MSG(marisa, "Eh? Maybe…");

	FACE(kurumi, dissatisfied);
	MSG(kurumi, "Huh?!");

	FACE(marisa, smug);
	MSG(marisa, "Your outfit’s missin’ somethin’. Fallen on hard times, maybe?");

	FACE(kurumi, tsun_blush);
	MSG(kurumi, "H-Hmph! I could say the same about you!");
	FACE(kurumi, normal);
	MSG(kurumi, "Seriously, it’s just a plain generic witch outfit!");

	FACE(marisa, normal);
	MSG(marisa, "My petticoats are legendary, y’know.");
	FACE(marisa, happy);
	MSG(marisa, "Girls across Gensōkyō talk about ‘em.");
	FACE(marisa, normal);
	MSG(marisa, "In fact, there’s a few of ‘em, uh, deeper inside this mansion.");
	FACE(marisa, happy);
	MSG(marisa, "So I’ll be headin’ in, then.");

	EVENT(music_changes);
	FACE(kurumi, tsun);
	MSG(kurumi, "Oh, I’m not letting you go without a fight!");
	FACE(kurumi, normal);
	MSG(kurumi, "Let’s see who’s the most fashionable here, witch!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	FACE(marisa, smug);
	MSG(marisa, "Whoever has the best petticoat wins. It’s a scientific fact.");
	FACE(marisa, sweat_smile);
	MSG(marisa, "… but since when do I care about scientific facts?");

	MSG(kurumi, "W-wait, what was I doing again?");
	MSG(kurumi, "(Y-yeah, that’s it, play it off as amnesia…");
	MSG(kurumi, "Uuu, I don’t remember anything at all!!");
	MSG(kurumi, "(But, hmm… haven’t I seen this nerdy-looking girl before…?)");

	FACE(marisa, normal);
	MSG(marisa, "Ah, sorry, sorry. I think ya slipped ‘n hit your head.");
	MSG(marisa, "I’m Gensōkyō’s… petticoat inspector.");
	MSG(marisa, "I’m here to inspect the petticoats in this here mansion.");
	FACE(marisa, happy);
	MSG(marisa, "Ya were just lettin’ me in, so I’ll be off, then.");

	FACE(kurumi, normal);
	MSG(kurumi, "Uh, I kinda doubt that…");
	FACE(kurumi, defeated);
	MSG(kurumi, "Not that I can stop you, anyways…");
	FACE(marisa, normal);
	MSG(kurumi, "Ugh, I didn’t sign up to get beaten like this!");

	FACE(marisa, puzzled);
	MSG(marisa, "‘Sign up’? I thought you didn’t remember anything.");

	FACE(kurumi, tsun_blush);
	MSG(kurumi, "N-Nevermind!");

	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(marisa, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(iku);
	HIDE(iku);

	FACE(marisa, inquisitive);
	MSG(marisa, "This place is fascinatin’!");
	MSG(marisa, "What kinda energy source would they even use for this?!");
	MSG(marisa, "I gotta tell Patchy!");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(iku, 180, "I would not recommend getting too enamoured, young witch.");
	SHOW(iku);
	MSG(iku, "Regardless, I’m surprised to see you here. How have you been?");

	MSG(marisa, "What grimoire would this be in…?");
	MSG(marisa, "Hmm… probably a major illusion of some kind…?");
	MSG(marisa, "That’d be easier than making the bricks weightless, ‘n it’s a good disguise…");

	TITLE(iku, "Nagae Iku", "Fulminologist of the Heavens");
	FACE(iku, smile);
	MSG(iku, "U-um, pardon me. Are you conversing with yourself?");

	FACE(marisa, normal);
	MSG(marisa, "Huh? No, I was askin’ ya a question.");

	MSG(iku, "Would that be your theory about this tower, perhaps?");

	MSG(marisa, "It’d make sense though, right?");
	FACE(marisa, inquisitive);
	MSG(marisa, "It appeared outta nowhere. If it were real brick, the air displacement alone woulda caused a huge noise…");
	MSG(marisa, "And it’d take so much power, too.");
	MSG(marisa, "Definitely a major illusion, then.");

	MSG(iku, "I feel as though the premise of your theory is flawed, unfortunately.");
	FACE(iku, eyes_closed);
	FACE(marisa, normal);
	MSG(iku, "The technology on display here is beyond our current comprehension.");

	FACE(marisa, surprised);
	MSG(marisa, "Technology? Whoa, so yer sayin’ this is REAL brick?!");
	FACE(marisa, normal);
	MSG(marisa, "*tap tap*");
	FACE(marisa, surprised);
	MSG(marisa, "It’s so cold it’s almost creepy, like it was suckin’ magic right outta my fingers.");
	FACE(marisa, inquisitive);
	MSG(marisa, "But that means the tower… has corporeal form?!");
	MSG(marisa, "Dang! Now I GOTTA meet the owner!");
	MSG(marisa, "What grimoires does she got to pull somethin’ like this off?!");

	FACE(iku, smile);
	MSG(iku, "S-she does not use grimoires, or magic.");
	FACE(iku, serious);
	MSG(iku, "She is relying on complex machinery, and likely advanced computational—");

	MSG(marisa, "But then, what’s the power source?");
	MSG(marisa, "Not even nuclear fusion could sustain somethin’ like this!");

	FACE(iku, smile);
	MSG(iku, "I-I am not sure of its exact mechanics—");

	FACE(marisa, happy);
	MSG(marisa, "Where’d y’say she was?");

	FACE(iku, normal);
	MSG(iku, "You mean…?");

	FACE(marisa, inquisitive);
	MSG(marisa, "Yeah, the owner! Upstairs, right?");

	FACE(iku, serious);
	MSG(iku, "Perhaps the thin atmosphere at this elevation has gotten to your head. I should not divulge any further information, for your own protection.");

	FACE(marisa, happy);
	MSG(marisa, "What’s her name?");

	MSG(iku, "Why are you not listening to me? You are not thinking straight!");

	FACE(marisa, inquisitive);
	MSG(marisa, "Do ya got any tips on talkin’ to her, maybe make her loosen up a bit?");
	FACE(marisa, happy);
	MSG(marisa, "I don’t wanna blow my chance.");

	FACE(iku, smile);
	MSG(iku, "I am trying to spare you further peril, yet all you can do is talk over me. Be more considerate.");
	MSG(iku, "I had expectations that you would be able to approach this with a clear head and resolve this incident, but it seems—");

	FACE(marisa, normal);
	MSG(marisa, "Ah, ah, I see. I’ll have to turn on the ol’ Kirisame charm.");
	FACE(marisa, smug);
	FACE(iku, serious);
	MSG(marisa, "I get it, I get it.");
	MSG(marisa, "Haven’t done ‘femme fatale’ in a while, but…");

	EVENT(music_changes);

	MSG(iku, "Enough!");
	MSG(iku, "It is said that specifically applied electro-stimulation can ease the mind.");
	MSG(iku, "Allow me to put that into practice for you!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);

	ACTOR_LEFT(marisa);
	FACE(marisa, surprised);

	// should be only one message with a fixed 180-frame (3 second) timeout
	MSG_UNSKIPPABLE(marisa, 180, "So this place runs on electricity, eh? Hmm…");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(iku);
	VARIANT(iku, defeated);
	FACE(iku, defeated);

	FACE(marisa, normal);
	MSG(marisa, "Oof, I hope that didn’t make my hair too frizzy.");
	FACE(marisa, happy);
	MSG(marisa, "Y’know, for my big first impression ‘n all.");

	FACE(iku, eyes_closed);
	MSG(iku, "F-For the last time, she’ll have nothing to teach you…");
	MSG(iku, "Her powers rely on technology and the scientific method. Of that much I am sure.");

	FACE(marisa, unamused);
	MSG(marisa, "The one time Sanae’s not around to geek out about all the sciencey stuff, eh?");
	FACE(iku, normal);
	FACE(marisa, normal);
	MSG(marisa, "Ah well, I know how to operate one of those Intelligent Phones, and that’s plenty advanced, so I should be fine.");
	FACE(marisa, inquisitive);
	MSG(marisa, "By the way, have ya been hearing that strange voice?");

	MSG(iku, "Strange voice?");

	FACE(marisa, smug);
	MSG(marisa, "Yeah, it keeps tellin’ me that I’m not thinkin’ straight or whatever.");
	MSG(marisa, "Probably just those weird madness rays tryin’ to get me.");

	FACE(iku, eyes_closed);
	MSG(iku, "Ugh, you fool.");
	MSG(iku, "But perhaps you are thinking just well enough to resolve this incident after all…");

	FACE(marisa, surprised);
	MSG(marisa, "Ah! There’s that voice again! So strange…");

	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(marisa, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(elly);
	HIDE(elly);

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(elly, 180, "I’ve been waiting for you to make your move.");
	SHOW(elly);
	MSG(elly, "Sneaking around as you are, and yet still so loud.");

	FACE(marisa, unamused);
	MSG(marisa, "Hmm…");

	FACE(elly, smug);
	MSG(elly, "Did you really think I wouldn’t notice all the commotion you caused inside the tower?");

	MSG(marisa, "Hmmmmmmm…");

	FACE(elly, angry);
	MSG(elly, "Stop being an insolent brat and speak up!");

	FACE(marisa, unamused);
	MSG(marisa, "… sigh.");

	FACE(elly, normal);
	MSG(elly, "… w-what is it?");

	FACE(marisa, unamused);
	MSG(marisa, "Ya don’t look like a magician after all.");
	MSG(marisa, "I’m pretty disappointed.");
	MSG(marisa, "Honestly, this might be the most disappointing incident yet.");

	TITLE(elly, "Elly", "The Theoretical Reaper");
	MSG(elly, "You’re after… magic?");
	FACE(elly, smug);
	MSG(elly, "Hah! You’ll regret wasting your time on such nonsense.");
	MSG(elly, "Nothing will stand against our path of vengeance for those who have been forgotten by the—");

	MSG(marisa, "Do ya know how boring it is to be a magician in Gensōkyō these days?");

	FACE(elly, eyes_closed);
	MSG(elly, "I couldn’t possibly care about—");

	MSG(marisa, "Seriously! Recently it’s just been ‘God this’ and ‘Otherworld that’.");
	MSG(marisa, "Reimu’s been pickin’ up some sweet skills, but me? Nothin’!");
	MSG(marisa, "Gods use magic too, but they’re like her! They don’t need books!");
	MSG(marisa, "But I do! And I end up empty handed every dang time!");

	FACE(elly, angry);
	MSG(elly, "I SAID I don’t CARE about your stupid little sorc—");

	MSG(marisa, "I saw this place ‘n I was all, ‘Whoa! Is this all enchantments ‘n stuff?!’");
	FACE(marisa, inquisitive);
	MSG(marisa, "I got really excited!");
	MSG(marisa, "‘Maybe I could get a new trump-card outta this’ is what I thought.");
	FACE(marisa, unamused);
	MSG(marisa, "But obviously yer a shinigami or somethin’. And I didn’t see a library…");
	MSG(marisa, "I guess that oarfish from before was right.");
	MSG(marisa, "Master Spark’s good ‘n all, but it’ll only get me so far in life…");

	FACE(elly, eyes_closed);
	MSG(elly, "…");
	MSG(elly, "Are you finally done?");
	FACE(elly, angry);
	MSG(elly, "Is this some kind of joke?!");
	FACE(elly, shouting);
	MSG(elly, "Are you just trying to mock me like you did the first time we met?!");

	MSG(marisa, "At least that time ya were guardin’ some powerful thingy I coulda used.");
	MSG(marisa, "I’m tryin’ to remember, was that time a bust, too?");
	FACE(marisa, happy);
	MSG(marisa, "At least I met Yūka, right? Did ya know she ended up givin’ me my trademark spell?");
	MSG(marisa, "Not right then, ‘n not voluntarily, but—");

	FACE(elly, angry);
	MSG(elly, "Y-you remember that?! But how—?!");
	MSG(elly, "… ugh, nevermind! It’s not as if you could possibly grasp the Tower’s true power.");
	FACE(elly, smug);
	MSG(elly, "After all, I can already sense your feeble mind succumbing to its might!");

	FACE(marisa, unamused);
	MSG(marisa, "Eh? I’m not depressed or anythin’. I’m just disappointed.");

	FACE(elly, shouting);
	MSG(elly, "I-It’s not about being ‘depressed’! It’s about being overwhelmed with the vast knowledge of the universe!");
	MSG(elly, "Once you realize the true potential of reality, you’ll go mad with knowledge!");
	MSG(elly, "Don’t you see?! Nothing can stop us now! For we are—!");

	FACE(marisa, surprised);
	MSG(marisa, "But I’m already mad with knowledge! Why do ya think I drink all the time?!");
	MSG(marisa, "My mind’s always racin’ with this ‘n that…");
	MSG(marisa, "Ya think this is any different than how I usually live?! Get over y’erself!");
	FACE(marisa, inquisitive);
	MSG(marisa, "Now, I gotta ask… ya really, seriously don’t got a secret library anywhere?");
	MSG(marisa, "A single bookshelf?! A few loose scribbled notes?!");
	MSG(marisa, "Heck, I’ll even take one of those little glowy tablet thingies with all those ‘Pee Dee Effs’ on ‘em! They hurt my dang eyes, but—");

	FACE(elly, eyes_closed);
	MSG(elly, "Since you’re OBVIOUSLY treating this as a joke, I REFUSE to speak with you any longer!");

	FACE(marisa, happy);
	MSG(marisa, "Oh c’mon, wanna catch up on old times?");
	MSG(marisa, "Turn the madness-whatever off ‘n we’ll go out for drinks! There’s this nice new bar in town with a cute—");

	MSG(elly, "There’s no force in this world that would make me turn back now!");
	EVENT(music_changes);
	FACE(elly, smug);
	MSG(elly, "Someone as ‘ordinary’ as you getting in our way is impressive, I’ll give you that much.");
	FACE(elly, angry);
	MSG(elly, "But there’s no place for sorcery in our enlightened vision of Gensōkyō!");
	FACE(elly, shouting);
	MSG(elly, "Now, succumb to the madness!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, angry);
	FACE(marisa, unamused);
	MSG(marisa, "Are ya sure ya don’t got a spellbook laying around?! Nothin’ at all?!");
	MSG(elly, "Are you still on about that?! Fine, see what good it does you!");
	FACE(elly, shouting);
	MSG(elly, "Your pitiful magic amounts to nothing against the true nature of reality!");

	DIALOG_END();
}

/*
 * Extra Stage
 */

DIALOG_TASK(marisa, StageExPreBoss) {
	DIALOG_BEGIN(StageExPreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(yumemi);
	HIDE(yumemi);

	FACE(marisa, inquisitive);
	MSG(marisa, "Whoa, very cool.");
	MSG(marisa, "What’s all that text floating out there? I can barely make it out…");

	MSG_UNSKIPPABLE(yumemi, 180, "It’s a computer programming language.");
	EVENT(boss_appears);
	SHOW(yumemi);
	FACE(yumemi, smug);
	MSG(yumemi, "… hmm, so it’s the witch scenario this time…");
	MSG(yumemi, "I’m afraid those languages aren’t of any use to someone like you.");
	MSG(marisa, "Oh yeah, you’re right! It’s that ‘C’? Maybe ‘Java’?");
	MSG(marisa, "It looks like it’s been put through a bunch of funky filters, I can’t make it out.");

	FACE(yumemi, surprised);
	MSG(yumemi, "Hmm? A fairytale witch who knows about computers?");

	FACE(marisa, happy);
	MSG(marisa, "Heh heh, I’ve helped the kappa out with computer problems tons of times!");

	MSG(yumemi, "Is that so?");
	FACE(yumemi, smug);
	MSG(yumemi, "How absurd.");
	MSG(yumemi, "But even knowing those things, this place is beyond the comprehension of any fantasy creature.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Those women I conscripted before are proof of that.");

	FACE(marisa, puzzled);
	MSG(marisa, "Hey, no point blamin’ students for a bad teacher.");

	MSG(yumemi, "…");
	FACE(yumemi, normal);
	MSG(yumemi, "I built this machine through the merging of computer science and unified physics.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "When it was completed, its impressive display of power and ability…");
	FACE(yumemi, sigh);
	MSG(yumemi, "… did nothing.");
	FACE(marisa, normal);
	FACE(yumemi, sad);
	MSG(yumemi, "Nobody cared. They were disillusioned before they'd even seen it.");
	MSG(yumemi, "‘Big deal. Rich people can already go to the moon. It’s just another toy for them.’");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "… even though I wouldn’t dare give those lunar-bound idiots even a glimpse.");

	FACE(yumemi, normal);
	FACE(marisa, puzzled);
	MSG(marisa, "Yeah, uh, screw those guys, I guess?");

	FACE(yumemi, smug);
	MSG(yumemi, "… heh.");
	FACE(yumemi, normal);
	MSG(yumemi, "You know, I knew people who used to practice witchcraft.");
	FACE(marisa, normal);
	MSG(yumemi, "Some of them even wore silly hats, just like yours.");
	FACE(yumemi, smug);
	MSG(yumemi, "I stopped talking to them once I realized the truth.");

	FACE(marisa, sweat_smile);
	MSG(marisa, "Aw, why?");
	FACE(marisa, happy);
	MSG(marisa, "Witches have got the best liquor, don’t ya know?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "We used to go to bars to meet them, actually.");
	FACE(marisa, normal);
	FACE(yumemi, sad);
	MSG(yumemi, "Those were simpler times…");

	FACE(marisa, puzzled);
	MSG(marisa, "‘We’? Ya mean those former-underlings of yours?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "…");
	FACE(yumemi, normal);
	MSG(yumemi, "It was all a distraction from the real problem.");
	FACE(marisa, normal);
	MSG(yumemi, "Comforting, at first, to think there was some hidden meaning to reality, but…");

	FACE(marisa, sweat_smile);
	MSG(marisa, "Hey, why don’t we work this all out over some sake and physics textbooks?");
	FACE(marisa, happy);
	MSG(marisa, "If you can look past the hat, I’m a pretty understandin’ lady!");

	FACE(yumemi, smug);
	MSG(yumemi, "I don’t take orders from fairytales.");

	FACE(marisa, normal);
	MSG(marisa, "Well, first off, I was askin’, not orderin’.");
	FACE(marisa, happy);
	MSG(marisa, "Second off, I’m a completely ordinary human.");

	FACE(yumemi, surprised);
	MSG(yumemi, "You’re a witch, aren't you?");

	MSG(marisa, "A completely ordinary witch.");
	FACE(marisa, normal);
	MSG(marisa, "Sake’s gettin’ cold.");
	MSG(marisa, "How about you do me a solid, turn this thing off, and you can tell me ALL about that Grand Unified Theory of yours back at my place?");

	FACE(yumemi, normal);
	MSG(yumemi, "If it’s all the same to you, I’d rather keep it on, and stay here.");
	FACE(marisa, happy);
	MSG(marisa, "Good thing it’s not all the same to me, then!");
	FACE(yumemi, surprised);
	MSG(yumemi, "Huh? It’s an expression-");
	MSG(marisa, "Gahahahah!");

	FACE(yumemi, sigh);
	MSG(yumemi, "I won’t take sass from some ‘ordinary’ witch.");
	FACE(marisa, smug);
	MSG(marisa, "Why not? My sass is legendary.");

	FACE(yumemi, normal);
	MSG(yumemi, "Because you’re not real. You’re a figment of delusional minds.");
	MSG(yumemi, "People waste their time thinking about ‘Otherworlds’, about Gensōkyō, while everything else decays and dies.");
	FACE(marisa, normal);
	MSG(yumemi, "I’m going to give them no other escape than the world they truly live in.");
	FACE(yumemi, sigh);
	MSG(yumemi, "It’s likely they’ll find some other distraction, some other way to destroy themselves…");
	FACE(yumemi, sad);
	MSG(yumemi, "It probably won’t change a thing.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "But at least I’ll have the satisfaction of having tried at all.");

	FACE(marisa, sweat_smile);
	MSG(marisa, "Yikes.");
	FACE(marisa, normal);
	MSG(marisa, "Y’know destroyin’ fantasy will just make your problems worse, right?");
	MSG(marisa, "If all I did was studyin’, I’d be dull in no time flat!");
	FACE(marisa, happy);
	MSG(marisa, "Goofin’ off and thinkin’ about weird stuff keeps your mind sharp! Let’s you recharge!");

	FACE(yumemi, smug);
	MSG(yumemi, "Yes, of course it’d work for the likes of you.");
	FACE(yumemi, normal);
	MSG(yumemi, "But for us mere humans, well, we’re unable to handle the Siren’s song of these places.");

	FACE(marisa, normal);
	MSG(marisa, "I really am human, y’know.");
	MSG(marisa, "And from one human to another, please, turn off the dang madness rays, would ya?");

	EVENT(music_changes);
	FACE(yumemi, sigh);
	MSG(yumemi, "Fantasy *is* tenacious, isn’t it? I guess it was always going to come down to a fight, no matter what.");
	MSG(yumemi, "It’s not like I have anything to lose.");
	FACE(yumemi, normal);
	MSG(yumemi, "If nothing else, destroying you will provide valuable data in annihilating the rest of you.");

	DIALOG_END();
}

DIALOG_TASK(marisa, StageExPostBoss) {
	DIALOG_BEGIN(StageExPostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(yumemi);
	VARIANT(yumemi, defeated);
	FACE(yumemi, defeated);

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Ugh…");

	FACE(marisa, smug);
	MSG(marisa, "What’s that thing the kappa keep sayin’? ‘Garbage in, garbage out’?");
	MSG(marisa, "I guess that makes ya the garbage? And now you’re gettin’… taken outside?");
	FACE(marisa, puzzled);
	MSG(marisa, "No, that’s ‘takin’ out the trash’…");

	FACE(yumemi, sad);
	MSG(yumemi, "God, you’re irritating…");

	FACE(marisa, smug);
	MSG(marisa, "‘God’? What’s a ‘God’ gotta do with it? Ain’t that against your non-beliefs?");
	MSG(marisa, "Or are ya just mad ya got schooled by an ‘imaginary’ girl wearin’ a silly hat?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "…");
	FACE(yumemi, normal);
	FACE(marisa, normal);
	MSG(yumemi, "Despite the extensive calculations, I still lost.");
	MSG(yumemi, "I suppose everything really was doomed from the start. People are too invested in fantasical delusions…");

	MSG(marisa, "Listen up, brain genius.");
	FACE(marisa, smug);
	MSG(marisa, "You’re doin’ mathematics and mathematicians a disservice by bein’ all hard-line like this.");
	FACE(marisa, happy);
	MSG(marisa, "Mathematicians are some of the weirdest folks out there!");
	FACE(marisa, inquisitive);
	MSG(marisa, "Ya ever just sit down ‘n really LOOK at the Mandelbrot Set?");
	FACE(marisa, happy);
	MSG(marisa, "How do ya think they come up with all that weird crap?");

	FACE(yumemi, surprised);
	MSG(yumemi, "Computer Science, and now Topology…?");
	FACE(marisa, normal);
	MSG(yumemi, "How is knowledge of things not utterly incompatible with your existence…?");

	FACE(marisa, inquisitive);
	MSG(marisa, "Wait, is it supposed to be?!");
	MSG(marisa, "Why didn’t ya say that before?!");
	FACE(marisa, smug);
	MSG(marisa, "I gotta run home ‘n burn all my Fractal Geometry textbooks right away!");
	FACE(marisa, happy);
	MSG(marisa, "Gahahaha!");

	FACE(yumemi, sigh);
	MSG(yumemi, "… okay, okay, I get it.");
	FACE(marisa, normal);
	MSG(yumemi, "I didn’t… expect to find somethi—");
	FACE(yumemi, normal);
	MSG(yumemi, "… someone, like you here.");

	MSG(marisa, "There’s somethin’ ya gotta learn, poindexter.");
	MSG(marisa, "Gensōkyō’s one of the weirdest places, anywhere, anywhen.");
	MSG(marisa, "Nothin’ here is like you’d expect, not even the unexpected!");
	FACE(marisa, happy);
	MSG(marisa, "Magic here’s all about mixin’ fantasy with reality. Always has been.");
	MSG(marisa, "It don’t matter where yer at - if you’re unbalanced about it, you’re gonna go nowhere in life.");
	FACE(marisa, smug);
	MSG(marisa, "And thaaaaat means, no friggin’ genocide, ya got it?!");

	FACE(marisa, normal);
	FACE(yumemi, sad);
	MSG(yumemi, "Is… there a point to this lecture? Aren’t you going to punish me anyways?");

	FACE(marisa, happy);
	MSG(marisa, "Gettin’ sassed on by me ain’t punishment enough? Gahahah!");

	MSG(yumemi, "Punish me with…");
	FACE(yumemi, sad);
	MSG(yumemi, "…");

	FACE(marisa, sweat_smile);
	MSG(marisa, "Whoa, whoa, for real? Ya think I’d do that?!");
	MSG(marisa, "Hey now, ya seem pretty miserable as it is. No point in addin’ to that misery.");
	FACE(marisa, smug);
	MSG(marisa, "How would I even explain that to the others?");
	MSG(marisa, "‘Oh yeah, I murdered that girl, no big deal’?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "I’m sorry.");
	FACE(marisa, normal);
	FACE(yumemi, sad);
	MSG(yumemi, "This was all a mistake. It was never going to solve anything anyways.");
	MSG(yumemi, "And… this isn’t what she—…");
	MSG(yumemi, "… ugh, I’ve been such an idiot!");

	FACE(marisa, normal);
	MSG(marisa, "Tragically stupid smart people are a dime a dozen in these parts.");
	FACE(marisa, happy);
	MSG(marisa, "You’ll fit right in!");

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER marisa
#include "export_dialog_tasks.inc.h"
