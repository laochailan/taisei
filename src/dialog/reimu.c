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

DIALOG_TASK(reimu, Stage1PreBoss) {
	PRELOAD {
		PRELOAD_CHAR(reimu) {
			PRELOAD_FACE(normal);
			PRELOAD_FACE(unamused);
			PRELOAD_FACE(sigh);
		}

		PRELOAD_CHAR(cirno) {
			PRELOAD_FACE(normal);
			PRELOAD_FACE(angry);
		}
	}

	// Initialization, must be at the very top (after PRELOAD).
	DIALOG_BEGIN(Stage1PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(cirno);

	// Hide cirno for now, to be revealed later.
	HIDE(cirno);

	// Let’s wait a bit, then change Reimu’s expression.
	// All timings are in frames (60 = 1 sec).
	WAIT_SKIPPABLE(60);
	// "normal" is the default face.
	FACE(reimu, normal);

	// MSG() makes the actor say a line, and then waits an unspecified amount of time (skippable).
	// The timeout is determined by the dialog_util_estimate_wait_timeout_from_text() function in dialog.c
	MSG(reimu, "Unseasonable snow? I wonder if it’s that ‘Secret God’ again…");

	// EVENT()s are handled by stage code.
	// You can find the list of events per dialogue in dialog_interface.h
	// All of them should be signaled eventually.
	EVENT(boss_appears);

	// MSG_UNSKIPPABLE() is like MSG(), but can’t be skipped and takes an explicit timeout.
	// Wait until the boss slides in.
	MSG_UNSKIPPABLE(cirno, 180, "Did you just call me a ‘God’?");

	// Reveal Cirno’s portrait
	SHOW(cirno);
	MSG(cirno, "What, are you *that* impressed by my incredible magic?!");

	FACE(reimu, sigh);
	MSG(reimu, "Right, of course, I completely forgot about you from before.");
	MSG(reimu, "Should’ve known.");

	// Titles are not yet implemented, but this should work once they are.
	// Right now this does nothing.
	TITLE(cirno, "Cirno", "Thermodynamic Ice Fairy");
	MSG(cirno, "Known better than to intrude on my secret lair!");

	FACE(reimu, normal);
	MSG(reimu, "Well, if *you’re* excited, I guess something really is going on.");
	MSG(reimu, "I’d ask if you’d just let me through, but…");

	EVENT(music_changes);
	FACE(cirno, angry);
	MSG(cirno, "No way! Prepare to be chilled to the bone!");

	// Teardown, must be at the very bottom.
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(cirno);

	// VARIANT() changes the base body sprite.
	// Bosses currently only have a "defeated" variant with some clothing damage, to be used in post-battle scenes.
	// Elly’s is called "beaten" and is slightly subdued.
	VARIANT(cirno, defeated);

	// Bosses also have a "defeated" face to go along with the variant, but all the other faces can be used as well.
	// It’s best to set the face to "defeated" in the beginning of a post-battle dialogue, and change it later if needed.
	FACE(cirno, defeated);
	MSG(cirno, "Cool down! I didn’t mean it like that!");
	FACE(reimu, smug);
	MSG(reimu, "I did say you could just let me through, you know.");
	MSG(cirno, "I don’t remember that! You broke the rules! I wasn’t ready yet…");

	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(reimu, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(hina);

	HIDE(hina);
	WAIT_SKIPPABLE(60);

	FACE(reimu, normal);
	MSG(reimu, "With those loudmouthed kappa gone, the mountain feels practically deserted.");
	FACE(reimu, sigh);
	MSG(reimu, "It’s almost like I miss them.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(hina, 180, "Ah, it’s Ms. Hakurei.");
	SHOW(hina);
	FACE(reimu, normal);
	FACE(hina, normal);
	MSG(hina, "I thought you might come.");
	FACE(hina, concerned);
	MSG(hina, "But once again, you’re trying to go places you shouldn’t.");
	MSG(hina, "What a troubled girl…");

	FACE(reimu, sigh);
	MSG(reimu, "So much for ‘deserted’…");
	FACE(reimu, normal);
	MSG(reimu, "Do you even know what you’re protecting me from this time?");

	TITLE(hina, "Kagiyama Hina", "Gyroscopic Pestilence God");

	FACE(hina, normal);
	MSG(hina, "A bright girl like you should know that Yōkai Mountain is especially dangerous today.");
	FACE(hina, concerned);
	MSG(hina, "You ought to turn back now, my dear.");

	FACE(reimu, sigh);
	MSG(reimu, "(She… she completely ignored my question.)");
	FACE(reimu, puzzled);
	MSG(reimu, "Does it matter that the Gods asked me to come this time?");

	EVENT(music_changes);

	MSG(hina, "In this instance, I’m sure I know better than the mountain Gods.");
	MSG(hina, "If you do not turn back immediately, I’ll have to repel you by force.");

	FACE(reimu, sigh);
	MSG(reimu, "So annoying…");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(hina);

	WAIT(30);

	VARIANT(hina, defeated);
	FACE(hina, defeated);
	FACE(reimu, normal);

	MSG(hina, "So I lost… you can keep going then, I suppose.");
	FACE(reimu, puzzled);
	MSG(reimu, "You should know by now that I can handle it. Why are you being so overbearing?");
	MSG(hina, "Just don’t say I didn’t warn you, when you inevitably get mauled by whatever’s out there…");

	FACE(reimu, unamused);
	MSG(reimu, "(Ignored again…)");

	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(reimu, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(wriggle);
	HIDE(wriggle);
	FACE(reimu, normal);
	MSG(reimu, "Aside from everyone being whipped up into a frenzy, I can’t see anything abnormal yet.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(wriggle, 180, "Well, to your human eyes, insects may seem ‘normal’…");

	SHOW(wriggle);
	FACE(wriggle, proud);
	MSG(wriggle, "… but there was a time when we ruled supreme!");

	FACE(reimu, puzzled);
	MSG(reimu, "Eh? You? Aren’t you a bit far from home?");
	MSG(reimu, "Besides, ‘ruled supreme’? I’ve never heard of insects ruling over Gensōkyō.");

	TITLE(wriggle, "Wriggle Nightbug", "Insect Rights Activist");
	FACE(wriggle, outraged);
	MSG(wriggle, "Home? But you’ve intruded on my secret lair!");

	FACE(reimu, sigh);
	MSG(reimu, "Again with the ‘secret lair’…");

	FACE(wriggle, proud);
	MSG(wriggle, "Haven’t you heard of the Car—…");
	MSG(wriggle, "Carb—…");
	FACE(reimu, puzzled);
	MSG(wriggle, "That period of history?");
	MSG(wriggle, "It was the greatest time, everyone should’ve heard of it by now!");

	MSG(reimu, "Seems pretty far-fetched to me, honestly. But you yōkai are always coming up with the craziest theories…");

	FACE(wriggle, outraged);
	MSG(wriggle, "Don’t lump us great insects in with common yōkai!");
	FACE(wriggle, proud);
	MSG(wriggle, "I’m going to restore us to our rightful glory of old!");

	FACE(reimu, puzzled);
	MSG(reimu, "Giant insects? But you’re tiny.");
	FACE(reimu, unamused);
	MSG(reimu, "And I still don’t know what you mean by ‘glory of old.’");

	EVENT(music_changes);

	MSG(wriggle, "You don’t seem very educated about our history.");
	MSG(wriggle, "So allow me to be your teacher!");
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(wriggle);

	VARIANT(wriggle, defeated);
	FACE(wriggle, defeated);

	FACE(reimu, smug);
	MSG(reimu, "Giant insects roaming Gensōkyō, huh?");
	MSG(reimu, "Sounds like one of Sanae’s ‘nuclear’ experiments.");

	MSG(wriggle, "B-but I saw it clearly… the fate of insectkind…");

	FACE(reimu, normal);
	MSG(reimu, "Don’t you have a pretty sweet deal as it is, though?");
	MSG(reimu, "You practically own Gensōkyō’s forests.");

	MSG(wriggle, "But can’t we ever hope for more…?");

	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(reimu, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(kurumi);

	HIDE(kurumi);

	FACE(reimu, puzzled);
	MSG(reimu, "Huh? There’s a mansion jutting out of this tower?");
	FACE(reimu, sigh);
	MSG(reimu, "This makes even less sense than usual.");

	EVENT(boss_appears);
	WAIT(30);

	FACE(reimu, normal);
	MSG(reimu, "And who are you supposed to be?");

	SHOW(kurumi);
	FACE(kurumi, tsun);
	MSG(kurumi, "Oi, that’s what I should be asking!");

	FACE(reimu, puzzled);
	MSG(reimu, "…?");
	FACE(reimu, unamused);
	MSG(reimu, "And what are you doing, anyways?");
	MSG(reimu, "Are you behind all these… strange thoughts everyone’s having?");

	FACE(kurumi, normal);
	TITLE(kurumi, "Kurumi", "High-Society Phlebotomist");
	MSG(kurumi, "I have no idea what you mean by ‘strange thoughts’, but I’ll be honest with you…");
	MSG(kurumi, "Your clothing is giving me some strange thoughts!");
	MSG(kurumi, "Where’d you even get that outfit?! Dumpster diving?! For real!");

	FACE(reimu, unamused);
	MSG(reimu, "Ugh, another prissy mansion-dweller.");
	FACE(reimu, sigh);
	MSG(reimu, "I thought I remembered you from somewhere, but no. Absolutely not.");
	FACE(reimu, assertive);
	MSG(reimu, "Just… stop whatever it is you’re doing, right now.");

	FACE(kurumi, dissatisfied);
	MSG(kurumi, "Once you get some fashion sense, maybe I will!");

	FACE(reimu, unamused);
	MSG(reimu, "… so you *are* behind this, huh?");
	FACE(reimu, sigh);
	MSG(reimu, "Besides, haven’t you ever seen a shrine maiden before?");

	FACE(kurumi, normal);
	MSG(kurumi, "You’re a shrine maiden?");
	FACE(reimu, unamused);
	MSG(kurumi, "More like… BAD maiden!");

	FACE(reimu, assertive);
	MSG(reimu, "T-this is what a shrine maiden wears. It’s a uniform.");

	MSG(kurumi, "Uniform? Are you kidding me?");
	MSG(kurumi, "I’ve seen a real shrine maiden before, and she didn’t look anything like you!");
	MSG(kurumi, "What’s with those patterns, anwyays? It’s completely unorthodox!");
	MSG(kurumi, "It’s not even using the right type of fabric! Get out of here, fake-maiden!");

	FACE(reimu, irritated);
	MSG(reimu, "… ARGH!");
	FACE(reimu, outraged);
	MSG(reimu, "Listen!! It’s HARD to find clothes I can wear every day on the job!");
	MSG(reimu, "It has to fit a certain style!");
	MSG(reimu, "And only a few fabrics feel good to wear!");
	MSG(reimu, "I can’t stand scratchy wool or slippery silk!");
	MSG(reimu, "Everything else feels terrible again my skin—");
	FACE(reimu, irritated);
	MSG(reimu, "… damnit! Now I’m all worked up!");

	EVENT(music_changes);

	MSG(reimu, "I hate prissy mansion dwellers!");
	FACE(reimu, outraged);
	MSG(reimu, "Let’s just get this over with!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	FACE(reimu, irritated);
	MSG(reimu, "That’s what you get for insulting my outfit like that.");

	MSG(kurumi, "Ouch ouch ouch…");
	MSG(kurumi, "W-wait, what happened?");

	MSG(reimu, "What do you mean ‘what happened’?");

	MSG(kurumi, "(M-maybe if I fake having amnesia… yeah, that’s it…!)");
	FACE(kurumi, normal);
	MSG(kurumi, "O-oof, my head! Ow! How did I even get here???");
	MSG(kurumi, "W-wait! Aren’t you that girl from a long while back?");

	FACE(reimu, unsettled);
	MSG(reimu, "I don’t remember you at all? It’s just that unsettling feeling again…");
	MSG(reimu, "So familiar…");
	FACE(reimu, sigh);
	MSG(reimu, "It’s annoying, whatever it is. Make it stop, or tell me how to make it stop.");

	FACE(kurumi, defeated);
	MSG(kurumi, "Huh? I-I don’t know! I barely remember anything!");
	MSG(kurumi, "(Ugh, her clothes really do suck though…)");

	MSG(reimu, "(Just another victim…?)");
	FACE(reimu, normal);
	MSG(reimu, "Stick around. I want to talk to you after I settle this.");

	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(reimu, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(iku);
	HIDE(iku);

	FACE(reimu, unsettled);
	MSG(reimu, "I’ve got a bad feeling about this…");
	FACE(reimu, puzzled);
	MSG(reimu, "Or maybe I’m just feeling nauseous with this tower’s constant spinning.");

	EVENT(boss_appears);
	WAIT(60);
	SHOW(iku);
	FACE(iku, normal);
	MSG(iku, "Or perchance it a side effect of the tower’s mere presence?");

	MSG(reimu, "Oh, it’s you. Why did you attack me earlier, anyway?");

	TITLE(iku, "Nagae Iku", "Fulminologist of the Heavens");
	FACE(iku, smile);
	MSG(iku, "Ah, apologies. It must have been friendly fire.");
	FACE(iku, eyes_closed);
	MSG(iku, "Things are quite hectic here. I beg your understanding.");
	MSG(iku, "But it has been a while, has it not?");

	FACE(reimu, normal);
	MSG(reimu, "Err, right, yes, it has been a while. Nice to see you again.");

	FACE(iku, serious);
	MSG(iku, "To get straight to the point, this tower is unlike anything I have seen before.");
	MSG(iku, "Since word spread through the clouds, I’ve learned that the culprit is connected to ‘parallel universes.’");

	FACE(reimu, puzzled);
	MSG(reimu, "You mean like Otherworlds? That’s not too out of the ordinary these days.");
	FACE(reimu, unsettled);
	MSG(reimu, "This feels different though. It’s like the walls are sucking up faith into a void.");
	MSG(reimu, "I’m surprised the fairies are surviving at all…");
	FACE(reimu, normal);
	MSG(reimu, "But what are you even doing here?");

	FACE(iku, eyes_closed);
	MSG(iku, "It is not just the Netherworld and Gensōkyō being affected.");
	FACE(iku, serious);
	MSG(iku, "In Bhava-agra, the Eldest Daughter’s personality has experienced some changes, too.");

	FACE(reimu, surprised);
	MSG(reimu, "It’s spreading that fast?!");

	FACE(iku, smile);
	MSG(iku, "Initially, I had no plans towards interfering.");
	FACE(reimu, normal);
	FACE(iku, serious);
	MSG(iku, "However, the tower’s psychological effects began to intensify, and I began to fear that nobody would take charge.");

	FACE(reimu, sigh);
	MSG(reimu, "(Do people have this little faith in me…?)");

	FACE(iku, eyes_closed);
	MSG(iku, "As more minds began to fall prey, I considered a theory…");
	FACE(reimu, normal);
	MSG(iku, "Is this machine incompatible with the existence of fantasy?");
	FACE(iku, normal);
	MSG(iku, "That may be why you feel a sense of dread about the place.");

	FACE(reimu, normal);
	MSG(reimu, "Well, that settles it. Where’s the owner, then?");

	FACE(iku, smile);
	MSG(iku, "To be frank, I am not sure whether you are up to the task.");
	MSG(iku, "You are still relatively composed, yes, but your mental state seems to be deteriorating.");

	FACE(reimu, irritated);
	MSG(reimu, "Deteriorating? But I just got here!");

	EVENT(music_changes);
	MSG(iku, "You are still so worrisome. You ought to go home before you lose yourself.");

	FACE(reimu, irritated);
	MSG(reimu, "No!");

	FACE(iku, serious);
	MSG(iku, "I beg your pardon?");

	MSG(reimu, "Don’t keep telling me how little you think of me. I’m sick of it!");
	FACE(reimu, assertive);
	MSG(reimu, "I’m going to give everyone a reason to have faith in me!");

	MSG(iku, "It seems your resolve is unwavering. Very well, then.");
	MSG(iku, "Allow me to test your resistance to their infuriating machine!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);

	ACTOR_LEFT(reimu);
	FACE(reimu, surprised);

	// should be only one message with a fixed 120-frames timeout
	MSG_UNSKIPPABLE(reimu, 120, "The oarfish? Why’s SHE here?");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(iku);
	VARIANT(iku, defeated);
	FACE(iku, defeated);

	MSG(iku, "Ah…");
	MSG(iku, "I was wrong for doubting you, I suppose.");

	FACE(reimu, unamused);
	MSG(reimu, "Somehow I don’t feel too satisfied by that.");

	FACE(iku, eyes_closed);
	MSG(iku, "To be honest with you, I’m glad you arrived after all.");
	MSG(iku, "I know now that I would not have been able to defeat her.");
	FACE(reimu, normal);
	MSG(iku, "Since you’re still able to think clearly, perhaps Gensōkyō will be saved after all.");

	FACE(reimu, sigh);
	MSG(reimu, "Finally.");
	FACE(reimu, normal);
	MSG(reimu, "But you’re right. This is bigger than Gensōkyō.");
	MSG(reimu, "I’ll need to take this seriously.");

	FACE(iku, normal);
	MSG(iku, "Good.");
	MSG(iku, "Keep climbing until you reach the top floor. You will undoubtedly meet the instigator there.");

	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(reimu, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(elly);
	HIDE(elly);
	EVENT(boss_appears);
	MSG_UNSKIPPABLE(elly, 180, "Well, well, well. It seems a rat trespassed on our domain.");
	SHOW(elly);

	FACE(elly, smug);
	MSG(elly, "How strange. You should’ve lost your mind by now.");

	FACE(reimu, unsettled);
	MSG(reimu, "Just turn it off already.");
	MSG(reimu, "I’m not in the mood to argue, so don’t push me!");

	TITLE(elly, "Elly", "The Theoretical Reaper");
	FACE(elly, angry);
	MSG(elly, "Bold of you to barge into MY tower and give ME orders!");
	MSG(elly, "I still remember what you did to Lady Kazami and I back then!");

	FACE(reimu, sigh);
	MSG(reimu, "And what was that, again?");
	MSG(reimu, "I’m getting a familiar feeling just looking at you, but…");

	FACE(elly, shouting);
	MSG(elly, "Is this a joke?! Are you trying to mock me?!");

	FACE(reimu, assertive);
	MSG(reimu, "I think I would’ve remembered a giant tower that makes people go mad with power!");
	MSG(reimu, "The least you can do when accusing someone is to tell them what they’ve supposedly done!");

	FACE(elly, smug);
	MSG(elly, "Ah. I see.");
	MSG(elly, "Fine. I will.");
	FACE(elly, normal);
	FACE(reimu, unamused);
	MSG(elly, "This tower is a recent invention. Its mere existence is revolutionary in several scientific fields.");
	MSG(elly, "The folks of this Gensōkyō ought to have been thankful for its vast scientific knowledge…");
	MSG(elly, "But alas, we have other plans.");

	FACE(reimu, unamused);
	MSG(reimu, "Somehow, it feels like more of a punishment.");
	MSG(reimu, "Ever heard of ‘ignorance is bliss’?");
	MSG(reimu, "There won’t be anyone to rule over, or whatever, if you make them all lose it.");

	MSG(elly, "The Tower of Babel is capable of ‘enlightening’ those fortunate to be around it.");
	FACE(elly, smug);
	MSG(elly, "Anyone unable to comprehend its brilliance will be driven to madness.");
	FACE(elly, shouting);
	MSG(elly, "But the residents of this Gensōkyō mean less than nothing to us! This is simply retribution!");

	FACE(reimu, unsettled);
	MSG(reimu, "‘This’ Gensōkyō…?");
	MSG(reimu, "‘Parallel universes’, is that it?");

	FACE(elly, normal);
	MSG(elly, "Hmm? Is that what you think this Gensōkyō is?");

	FACE(reimu, irritated);
	MSG(reimu, "You tell me! You’re the megalomaniac!");
	FACE(reimu, assertive);
	MSG(reimu, "Do you hate me because some other Reimu did something? Is that it?");
	MSG(reimu, "But then what does that have to do with any of us?!");
	FACE(reimu, irritated);
	MSG(reimu, "For a ‘scientist’, you’re not making a whole lot of sense!");

	FACE(elly, angry);
	MSG(elly, "Perhaps you’ve simply forgotten, like you have of the rest of us!");
	FACE(reimu, unamused);
	MSG(elly, "Only the privileged, such as Lady Kazami, made it out! The rest of us were lost!");

	FACE(reimu, unsettled);
	MSG(reimu, "You mean Kazami Yūka? And I suppose… Alice, too?");
	MSG(reimu, "I remember when I first met Alice, and she said weird things too, just like you are now.");
	FACE(reimu, unamused);
	MSG(reimu, "But she didn’t go around threatening everyone!");

	FACE(elly, angry);
	MSG(elly, "We weren’t as lucky! And as reparations, we will take control of this Gensōkyō!");
	MSG(elly, "We will make it so we will NEVER be forgotten!");

	FACE(reimu, sigh);
	MSG(reimu, "Even in Gensōkyō, you can be forgotten, huh?");
	MSG(reimu, "What a miserable thought. No wonder I felt terrible just seeing this place!");
	FACE(reimu, assertive);
	MSG(reimu, "Turn it off, or being forgotten will be the least of your worries!");

	EVENT(music_changes);
	FACE(elly, shouting);
	MSG(elly, "It’s too late for that! Soon, all of Gensōkyō will become enlightened!");
	FACE(reimu, unsettled);
	MSG(elly, "Don’t you understand, pitiful shrine maiden? Science is an unfathomable power, surpassed by nothing!");
	MSG(elly, "We won’t be forgotten by this world, at any cost!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, shouting);
	FACE(reimu, assertive);
	MSG(reimu, "Is that it? Are you finally done?");
	MSG(elly, "Y-you think this is over?! The time of reckoning is upon you!");
	MSG(elly, "Gaze upon the ultimate truth of the universe, and tremble!");

	DIALOG_END();
}

/*
 * Extra Stage
 */

#include "stages/stagex/stagex.h"

DIALOG_TASK(reimu, StageExPreBoss) {
	DIALOG_BEGIN(StageExPreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(yumemi);
	HIDE(yumemi);
	yumemi.draw_dynamic_overlay = stagex_draw_yumemi_portrait_overlay;

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(yumemi, 180, "So it's this scenario that's playing out, hmm?");
	SHOW(yumemi);
	FACE(yumemi, surprised);
	MSG(yumemi, "The impossible shrine maiden has finally arrived.");
	FACE(yumemi, smug);
	MSG(yumemi, "Here to set me on the ‘right path’, protagonist girl?");
	MSG(yumemi, "Just like in all of your other tall tales…");

	FACE(reimu, unsettled);
	MSG(reimu, "I get the worst feeling looking at you.");

	FACE(yumemi, surprised);
	MSG(yumemi, "Oh? How’s that?");

	FACE(reimu, sigh);
	MSG(reimu, "It’s mostly just how miserable you look.");

	FACE(yumemi, normal);
	MSG(yumemi, "Do I look especially miserable? Hmm.");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "And what would you know about misery, living in this fantastical paradise?");

	FACE(reimu, unamused);
	MSG(yumemi, "Have the other fairytale creatures hurt your feelings?");

	FACE(yumemi, normal);
	FACE(reimu, unsettled);
	MSG(reimu, "*Other* fairytale creatures?");

	FACE(reimu, unamused);
	MSG(reimu, "I’m human, you know.");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Oh, please. You’re an invincible, flying, magic-wielding shrine maiden—");
	FACE(yumemi, normal);

	FACE(reimu, sigh);
	MSG(reimu, "I don’t use magic.");

	FACE(yumemi, surprised);
	MSG(yumemi, "Hah.");
	FACE(yumemi, smug);
	FACE(reimu, irritated);
	MSG(yumemi, "Hahaha.");
	FACE(yumemi, normal);
	MSG(yumemi, "Perhaps you're unaware, but the world - the real world - is dying, shrine maiden.");

	FACE(yumemi, sad);
	FACE(reimu, unamused);
	MSG(yumemi, "People pour their hearts and souls into fantasy, leaving none of that energy, that drive, for the real world.");
	MSG(yumemi, "People talk of ‘solutions’ and ‘progress,’ but it’s pointless so long as people wile away their lives in useless places like this.");

	FACE(reimu, puzzled);
	MSG(reimu, "How’s that Gensōkyō’s problem?");
	FACE(reimu, unamused);
	MSG(reimu, "It’s not like we invaded you and forced you into it or anything.");

	FACE(yumemi, sigh);
	MSG(yumemi, "If only it were that simple.");
	FACE(yumemi, normal);
	FACE(reimu, unamused);
	MSG(yumemi, "Places like this tempt people into forgetting their real lives.");
	FACE(yumemi, sad);
	MSG(yumemi, "And by abandoning the real world, they doom not just themselves, but everyone around them.");

	FACE(reimu, unamused);
	MSG(reimu, "People are allowed to have a break from reality.");
	FACE(reimu, puzzled);
	MSG(reimu, "Haven’t you ever read a book?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "You mean fiction? I haven’t had a need for it in years.");
	FACE(yumemi, sigh);
	MSG(yumemi, "It’s a waste of time.");

	FACE(reimu, unsettled);
	MSG(reimu, "Wow, no wonder you’re so miserable.");

	FACE(yumemi, sad);
	MSG(yumemi, "Even as I introduced this technology to the world, nobody woke up from their dissociative haze. It was as it ever was.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "But at least, with this machine I’ve built, I can erase every so-called ‘Otherworld’ from the fabric of social reality.");
	FACE(yumemi, normal);

	FACE(reimu, assertive);
	MSG(reimu, "People won’t stop day-dreaming just because you tell them to. Fantasy is in people’s hearts and minds.");
	FACE(reimu, sigh);
	MSG(reimu, "So, even if you managed to destroy Gensōkyō—");

	MSG(yumemi, "Yōkai, Gods, magicians, flying shrine maidens…");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "It’s all delusional nonsense.");

	FACE(reimu, unsettled);
	MSG(reimu, "So you want genocide?");

	FACE(yumemi, surprised);
	MSG(yumemi, "Genocide…? What a harsh word.");
	FACE(yumemi, normal);
	FACE(reimu, unamused);
	MSG(yumemi, "You’re not real. No one is being killed.");

	MSG(reimu, "No wonder those two got freaked out just thinking about you. You’re completely unreasonable.");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Ah, I thought I’d made them both see things my way, by introducing them to the Grand Unified Theory.");
	FACE(yumemi, normal);
	MSG(yumemi, "But in the end, they just betrayed me.");
	FACE(yumemi, sad);
	MSG(yumemi, "In hindsight, it makes sense, of course. They can’t betray what they are.");

	FACE(reimu, irritated);
	MSG(reimu, "It’s hard to keep people on your side when all you do is threaten them, idiot!");
	FACE(reimu, assertive);
	MSG(reimu, "Have you tried not being a genocidal maniac?");

	FACE(yumemi, sigh);
	MSG(yumemi, "I’m tired, shrine maiden. Too tired to explain my life to a figment of imagination.");
	FACE(reimu, unamused);
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Perhaps I’ve already given up trying to save my society, too.");

	FACE(reimu, assertive);
	MSG(reimu, "Sucks to be you. Don’t make it our problem.");

	EVENT(music_changes);

	FACE(yumemi, normal);
	MSG(yumemi, "Oh, but it’s inherently your problem, isn’t it? Your very existence is the root of my misery.");
	FACE(reimu, unsettled);
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Farewell, shrine maiden. I would say it’s been nice knowing you, but…");
	FACE(yumemi, sad);
	MSG(yumemi, "There’s really no way I could’ve known you at all.");

	DIALOG_END();
}

DIALOG_TASK(reimu, StageExPostBoss) {
	DIALOG_BEGIN(StageExPostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(yumemi);
	yumemi.draw_dynamic_overlay = stagex_draw_yumemi_portrait_overlay;

	VARIANT(yumemi, defeated);
	FACE(yumemi, defeated);
	MSG(yumemi, "I yield.");

	FACE(reimu, sigh);
	MSG(reimu, "Finally.");

	FACE(yumemi, sad);
	MSG(yumemi, "Even with the full computational capacity of this machine, I wasn’t able to calculate a winning trajectory…");
	FACE(reimu, unsettled);
	MSG(yumemi, "Nor did any of you succumb to the truth.");
	FACE(yumemi, sigh);
	MSG(yumemi, "I suppose it really was hopeless all along.");

	FACE(reimu, puzzled);
	MSG(reimu, "What ‘truth’?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "That this isn’t real.");
	MSG(yumemi, "That it’s fantasy, a delusion.");
	MSG(yumemi, "That faith and magic—");

	FACE(reimu, unsettled);
	MSG(reimu, "That faith and magic don’t exist?");
	MSG(reimu, "That we’re no different than legends and myths?");
	MSG(reimu, "That if the barrier separating Gensōkyō from the Outside World were to fall, I’d lose my powers, or worse, and that all yokai would go extinct?");

	FACE(reimu, assertive);
	MSG(reimu, "We knew all of that already! Everyone here does!");

	FACE(yumemi, surprised);
	MSG(yumemi, "Are… are you serious? But then, how could you—");

	FACE(reimu, irritated);
	MSG(reimu, "Outside World humans are so irritating!");
	MSG(reimu, "Did you even bother talking to anyone when you got here?!");
	MSG(reimu, "No, of course not! Why would you?! You’re too full of yourself to ask anyone about anything!");
	FACE(reimu, assertive);
	FACE(yumemi, sad);
	MSG(reimu, "What, was your big plan to use those ‘madness rays’ or whatever it was to make us all ‘think ourselves out of existence’?");

	MSG(yumemi, "But how—");

	FACE(reimu, irritated);
	MSG(reimu, "That never would’ve worked! We’re always thinking about how precarious Gensōkyō is!");
	MSG(reimu, "What do you think my job *is*, exactly?");
	FACE(reimu, assertive);
	MSG(reimu, "All you really did was make everyone act all chu—…");

	FACE(reimu, unamused);
	MSG(reimu, "Chuu—…");

	FACE(reimu, irritated);
	FACE(yumemi, surprised);
	MSG(reimu, "…ugh, it’s that weird word Sumireko keeps using…");

	FACE(reimu, assertive);
	MSG(reimu, "Y-You just made everyone act even more delusional than they already are!");

	MSG(yumemi, "Sumire…ko?");
	MSG(yumemi, "Usami Sumireko?");

	FACE(reimu, puzzled);
	MSG(reimu, "Huh? You know her or something?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "… You’re right. I’ve been a complete idiot.");
	MSG(yumemi, "I surrender. I’ll shut everything down.");

	FACE(reimu, puzzled);
	MSG(reimu, "What?! That’s all it took to talk you down?!");
	FACE(reimu, assertive);
	MSG(reimu, "That’s infuriatingly random, you know!");

	FACE(reimu, sigh);
	MSG(reimu, "Maybe I should’ve talked her into it and saved me all this trouble!");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Do you know if…");

	FACE(yumemi, sad);
	FACE(reimu, puzzled);
	MSG(yumemi, "… no. I doubt she’d be here too. It’d be too convenient.");
	FACE(reimu, unamused);
	MSG(yumemi, "I’ve failed. I accept your punishment.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Finish me off. I won’t resist.");

	FACE(reimu, puzzled);
	MSG(reimu, "‘Finish you off’?");
	MSG(yumemi, "End my life. I threatened your world with ‘genocide’, didn’t I?");
	FACE(reimu, unamused);
	MSG(reimu, "Huh? But who would you even be a martyr for? You don’t have any followers left.");

	FACE(yumemi, sad);
	MSG(yumemi, "What—?");

	FACE(reimu, sigh);
	MSG(reimu, "Ugh, I’ll let the Moriya Gods deal with you.");
	MSG(reimu, "Be prepared to give endless lectures about that ‘Grand Unified Whatever’.");

	FACE(yumemi, normal);
	MSG(yumemi, "The ‘Grand Unified Theory’?");
	FACE(reimu, unamused);
	MSG(yumemi, "That’s… my punishment for all this? To give physics lectures?");

	FACE(reimu, assertive);
	MSG(reimu, "Do something useful with that big brain of yours for once!");
	FACE(reimu, irritated);
	MSG(reimu, "Count yourself lucky that Kanako probably won’t put you on fairy vomit duty!");

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER reimu
#include "export_dialog_tasks.inc.h"
