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

DIALOG_TASK(youmu, Stage1PreBoss) {
	DIALOG_BEGIN(Stage1PreBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(cirno);
	HIDE(cirno);

	FACE(youmu, happy);
	MSG(youmu, "I suppose falling snow can be just as pretty as cherry blossoms…");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(cirno, 180, "Hey!");
	SHOW(cirno);

	MSG(cirno, "I’m cooler than you, so get off my lake!");
	FACE(youmu, unamused);
	MSG(youmu, "I can see you’re quite cold, yes.");
	FACE(youmu, smug);
	MSG(youmu, "I tend to dislike dueling those weaker than myself, so I’d appreciate it if you’d step aside.");

	TITLE(cirno, "Cirno", "Thermodynamic Ice Fairy");
	MSG(cirno, "Hah! Maybe YOU should step aside, then!");

	FACE(youmu, eyes_closed);
	MSG(youmu, "I suppose it’s as good a time as any to practice my Snowflake-Destruction Technique.");

	EVENT(music_changes);
	FACE(cirno, angry);
	MSG(cirno, "But if I turn you into a snowflake, you’ll have to chop yourself in half!");

	DIALOG_END();
}

DIALOG_TASK(youmu, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(cirno);
	VARIANT(cirno, defeated);
	FACE(cirno, defeated);

	FACE(youmu, sigh);
	MSG(youmu, "The cold is no good for such fine blades…");

	MSG(cirno, "Y-you should be careful with t-those!");

	FACE(youmu, unamused);
	MSG(youmu, "Yes, that’s what I was just saying.");

	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(youmu, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(hina);
	HIDE(hina);

	FACE(youmu, normal);
	MSG(youmu, "So this is Yōkai Mountain…");
	MSG(youmu, "A shame, I’ve heard this is usually quite a lively place.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(hina, 180, "Oh my, what’s this? A new face?");
	SHOW(hina);
	MSG(hina, "And who might you be, dear?");

	MSG(youmu, "Konpaku Yōmu. I am on a mission from Lady Yuyuko to investigate the disturbance.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "You needn’t concern yourself. I’ll be passing through now.");

	TITLE(hina, "Kagiyama Hina", "Gyroscopic Pestilence God");
	MSG(hina, "I’ve never heard of this ‘Lady Yuyuko’ you speak of, but I can’t let this slide.");
	FACE(youmu, normal);
	FACE(hina, serious);
	MSG(hina, "Young girls such as yourself shouldn’t be sent to do such dangerous errands!");

	FACE(youmu, embarrassed);
	MSG(youmu, "‘Y-Young girl’?! I’ll have you know that I am an expert swordswoman—");
	MSG(youmu, "—having mastered my technique over *decades*—");
	MSG(youmu, "—dare I say even the best in Gensōkyō—");

	FACE(hina, normal);
	MSG(hina, "Aww… but you look so cute! No way someone like you could be into that.");
	MSG(hina, "Are you sure this ‘expert swordswoman’ thing isn’t just some sort of teenage phase?");

	FACE(youmu, unamused);
	MSG(youmu, "It’s not a PHASE, mo—");
	FACE(youmu, embarrassed);
	MSG(youmu, "…!!");

	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(youmu, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(hina);
	VARIANT(hina, defeated);
	FACE(hina, defeated);

	MSG(hina, "Did you just call me ‘mom’…?");

	FACE(youmu, eyes_closed);
	MSG(youmu, "I did not.");
	MSG(youmu, "It must be the madness overcoming you.");
	MSG(youmu, "Yes, indeed.");

	FACE(hina, normal);
	MSG(hina, "My, my…~");

	FACE(youmu, unamused);
	MSG(youmu, "Don’t you dare.");

	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(youmu, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(wriggle);
	HIDE(wriggle);
	FACE(youmu, relaxed);
	MSG(youmu, "The further I go into this forest, the more restless I feel.");
	FACE(youmu, normal);
	MSG(youmu, "Something is definitely amiss.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(wriggle, 120, "Amiss?");
	SHOW(wriggle);
	MSG(wriggle, "Perhaps you’re frightened by our growing, overwhelming population?");

	FACE(youmu, eyes_closed);
	MSG(youmu, "Bugs do not concern me at the moment.");

	FACE(wriggle, outraged);
	MSG(wriggle, "Oh, so all insects are ‘bugs’ now, hmm? I see how it is!");
	TITLE(wriggle, "Wriggle Nightbug", "Insect Rights Activist");
	MSG(wriggle, "Ever heard of beetles? Flies? It’s about respect, you know!");

	FACE(youmu, normal);
	MSG(youmu, "Apologies, I meant no disrespect.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "I am on a mission from Lady Yuyuko to end whatever is causing the disturbance in this area.");
	FACE(youmu, normal);
	MSG(youmu, "Tell me everything you know.");

	FACE(wriggle, proud);
	MSG(wriggle, "Oh, I understand now.");
	FACE(wriggle, calm);
	MSG(wriggle, "The power wielded by an army of giant insects would surely raise some concerns.");
	FACE(wriggle, proud);
	MSG(wriggle, "Simply put, we want to restore our rightful place in history!");
	MSG(wriggle, "It’s only natural that the lower inhabitants of the mountain would get nervous in the face of our natural dominance.");
	FACE(wriggle, calm);
	MSG(wriggle, "Your apology is accepted. You shall not fear our wrath. For now.");

	FACE(youmu, unamused);
	MSG(youmu, "… no, no, bugs alone wouldn’t cause this kind of disturbance…");
	FACE(youmu, eyes_closed);
	MSG(youmu, "All they’re capable of is ruining my carefully manicured garden.");

	MSG(wriggle, "Did you just say you’re…");
	FACE(wriggle, outraged);
	MSG(wriggle, "… a gardener?!");

	EVENT(music_changes);

	FACE(youmu, puzzled);
	MSG(wriggle, "Your true face is finally revealed!");
	MSG(wriggle, "My light shines brighter than ever now! I shall teach this lowly usurper due respect!");
	MSG(wriggle, "So that we may live above beyond your green thumb once and for all!");

	DIALOG_END();
}

DIALOG_TASK(youmu, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(wriggle);
	VARIANT(wriggle, defeated);
	FACE(wriggle, defeated);

	FACE(youmu, eyes_closed);
	MSG(youmu, "Are you aware of the legend of the Tonbokiri?");
	MSG(youmu, "It was said its blade could cut insects in half simply by them landing on it.");
	FACE(youmu, smug);
	MSG(youmu, "Undoubtedly, that is what has transpired today.");

	MSG(wriggle, "That’s not the past I saw…");

	MSG(youmu, "How unfortunate for you.");
	FACE(youmu, chuuni);
	MSG(youmu, "Fear not. I will end your delusional madness and restore justice and order to the—");
	FACE(youmu, eeeeh);
	MSG(youmu, "… ugh, that’s a little much, even for me, isn’t it?");

	MSG(wriggle, "Seems about right, honestly…");

	FACE(youmu, unamused);
	MSG(youmu, "…");

	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(youmu, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(kurumi);
	HIDE(kurumi);

	MSG(youmu, "A-ha, I must be close to the true entrance now.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "This tower… I can already feel it…");
	FACE(youmu, chuuni);
	MSG(youmu, "Can this truly be a challenge worthy of my skills, at long last?");

	EVENT(boss_appears);
	SHOW(kurumi);
	MSG(kurumi, "This mansion is actually pretty trashy, though. I expected better.");
	FACE(youmu, relaxed);
	MSG(youmu, "Who might you be?");

	TITLE(kurumi, "Kurumi", "High-Society Phlebotomist");
	FACE(kurumi, tsun);
	MSG(kurumi, "Hmph! I’m Kurumi, a lone black rose drowning in a sea of bad taste!");
	MSG(kurumi, "You better remember that!");

	FACE(youmu, eyes_closed);
	MSG(youmu, "Perhaps I will.");
	FACE(youmu, normal);
	MSG(youmu, "Are you an associate of this… mansion-tower’s denizens, then?");

	MSG(kurumi, "Of course not! Who do you take me for?!");
	MSG(kurumi, "No WAY I’d want to associate myself with the folk of this mansion!");
	MSG(kurumi, "They dress like they’ve fallen out of a bad 80s anime!");

	FACE(youmu, eyes_closed);
	MSG(youmu, "I see. I have no immediate reason to distrust you.");
	FACE(youmu, puzzled);
	MSG(youmu, "… what’s an ‘80s anime’, though, if I may ask?");

	FACE(kurumi, normal);
	MSG(kurumi, "I don’t know, don’t ask me!");
	MSG(kurumi, "The lady in the tower talks about it a lot, so it must be something horrible for sure.");

	FACE(youmu, unamused);
	MSG(youmu, "Didn’t you just say you weren’t associated with them?");

	MSG(kurumi, "Yeah, but I was lying to see how gullible you were.");

	FACE(youmu, embarrassed);
	MSG(youmu, "O-of course. I-I knew that.");
	FACE(youmu, normal);
	MSG(youmu, "I wanted to see if you’d come clean.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "My true challenge lies beyond you. Step aside.");

	EVENT(music_changes);

	MSG(kurumi, "Nope. You’ve got a dance with a gorgeous vampire scheduled first!");
	MSG(kurumi, "En garde, brooding swordsgirl!");

	DIALOG_END();
}

DIALOG_TASK(youmu, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	FACE(youmu, eyes_closed);
	MSG(youmu, "Despite your inferior technique, I must thank you.");
	MSG(youmu, "‘Dancing’ with you has been a pleasant distraction on the path to completing my mission.");
	FACE(youmu, chuuni);
	MSG(youmu, "Tell your ‘leader’ that I am coming for them.");

	MSG(kurumi, "(She earnestly talks like a manga anti-hero…)");
	FACE(kurumi, normal);
	MSG(kurumi, "Oh, okay then!");
	FACE(kurumi, defeated);
	MSG(kurumi, "I’m sick of guarding this creepy spooky mansion anyways.");

	FACE(youmu, unamused);
	MSG(youmu, "S-… spooky?");

	FACE(kurumi, puzzled);
	MSG(kurumi, "…?");
	FACE(kurumi, normal);
	MSG(kurumi, "… yeah! I thought it was gonna be all ‘scientific,’ but then they made it haunted, with ghosts and stuff!");

	FACE(youmu, eeeeh);
	MSG(youmu, "G-…");
	MSG(youmu, "GHOSTS?!");

	MSG(kurumi, "Scary vengeful ghosts! Trying to kill you, even!");
	MSG(kurumi, "But surely a strong swordsgirl like yourself could handle it.");

	FACE(youmu, embarrassed);
	MSG(youmu, "B-Bring it on! Nothing will stand in my way!");

	MSG(kurumi, "(Teehee, so gullible.)");

	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(youmu, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(iku);
	HIDE(iku);

	FACE(youmu, relaxed);
	MSG(youmu, "Incredible. Truly astounding.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "Yes, the final battle approaches. I can feel it.");
	FACE(youmu, chuuni);
	MSG(youmu, "Will it be a legendary duel? One to go down in the history of Gensōkyō?!");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(iku, 180, "Oh, the gardener from the Netherworld?");
	SHOW(iku);
	MSG(iku, "It’s no surprise to see you here.");
	MSG(iku, "Your mistress has been taken by the tower’s presence, I assume?");

	FACE(youmu, normal);
	MSG(youmu, "Lady Yuyuko is in a safe, undisclosed location.");

	MSG(iku, "The Hakurei Shrine, I presume?");

	FACE(youmu, surprised);
	MSG(youmu, "Wh-who told you?! Are you in league with them too?!");

	FACE(iku, smile);
	MSG(iku, "Of course not, but where else would she be at a time like this?");

	FACE(youmu, unamused);
	MSG(youmu, "Hmph. Perhaps we have become too predictable.");
	MSG(youmu, "I am here to settle this matter. You need not concern yourself with it any longer.");

	FACE(iku, normal);
	MSG(iku, "On the contrary, I would say.");
	TITLE(iku, "Nagae Iku", "Fulminologist of the Heavens");
	FACE(youmu, normal);
	MSG(iku, "I decided to do some research on my own.");
	MSG(iku, "This tower is controlled by a machine…");
	FACE(iku, serious);
	MSG(iku, "… and the machine’s effects seem to go well beyond Gensōkyō’s boundaries.");

	FACE(youmu, normal);
	MSG(youmu, "Including the heavens, I presume? I see.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "Fear not. I will allow nothing to stand in my way.");
	FACE(youmu, chuuni);
	MSG(youmu, "Konpaku Yōmu, the greatest swordswoman to live and die, will settle this incident once and for all!");

	FACE(iku, smile);
	MSG(iku, "It seems that the machine has also overinflated your ego, Swordswoman of the Hakugyokurō.");
	MSG(iku, "Would caution not be prudent in this situation?");

	FACE(youmu, eyes_closed);
	MSG(youmu, "Overinflated ego? Nonsense.");
	MSG(youmu, "During my recent battles, I have achieved a meditative state most blademasters only dream of!");
	FACE(youmu, chuuni);
	MSG(youmu, "Through that, I have realized the true form of my previously-incomplete yōkai-vanquishing technique!");

	MSG(iku, "Is that so? You believe that this ‘technique’ will help you defeat the instigators effortlessly?");

	FACE(youmu, sigh);
	MSG(youmu, "More doubt from ignorant skeptics, I see.");
	FACE(youmu, chuuni);
	MSG(youmu, "Shall we duel so that I may demonstrate to your satisfaction?!");

	FACE(iku, serious);
	MSG(iku, "W-wait, that is not what I meant, I simply—");

	EVENT(music_changes);

	MSG(iku, "Ah, I haven’t the time for this nonsense!");
	MSG(iku, "Insolent humans such as you need rehabilitation! Allow me to administer some onto you!");

	MSG(youmu, "Finally! Witness the overwhelming strength of my Rōkanken!");

	DIALOG_END();
}

DIALOG_TASK(youmu, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);

	ACTOR_LEFT(youmu);
	FACE(youmu, smug);

	// should be only one message with a fixed 180-frames timeout
	MSG_UNSKIPPABLE(youmu, 180, "Lightning? Hmph. I can outrun it with ease.");

	DIALOG_END();
}

DIALOG_TASK(youmu, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(iku);
	VARIANT(iku, defeated);
	FACE(iku, defeated);

	FACE(youmu, eyes_closed);
	MSG(youmu, "All according to plan.");

	MSG(iku, "Your logic is so twisted…");
	MSG(iku, "Alas, it cannot be helped. You have likely been affected since before your arrival.");
	MSG(iku, "I have no choice but to let you through, despite my reservations.");
	MSG(iku, "Perhaps pure intentions lurk behind that ego of yours.");

	MSG(youmu, "And perhaps I truly have gone mad. What of it?");
	MSG(youmu, "My mission is crystal clear:");
	FACE(youmu, chuuni);
	MSG(youmu, "… to carry out Lady Yuyuko’s mission, and to bring peace to Gensōkyō and all connected realms!");
	MSG(youmu, "I will not fail!");

	MSG(iku, "I-I see.");
	MSG(iku, "Ascend the staircase. You will find the perpetrator waiting for you there.");
	MSG(iku, "Be prepared. The technology she possesses is from a so-called ‘parallel universe’.");
	MSG(iku, "It is unlike anything any of us have encountered before. Even I am not sure of its specifics.");

	FACE(youmu, eyes_closed);
	MSG(youmu, "Fear not, I will slice her infernal machine in half like an invasive weed.");

	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(youmu, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(elly);
	HIDE(elly);

	FACE(youmu, eyes_closed);
	MSG(youmu, "Ah.");
	FACE(youmu, chuuni);
	MSG(youmu, "Ah-hahaha!");
	MSG(youmu, "Yes. Yes! This is it!");
	FACE(youmu, happy);
	MSG(youmu, "This is obviously the dramatic arena for the final battle!");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(elly, 180, "Nothing seems to slow you down, hmm?");

	SHOW(elly);
	MSG(elly, "You’re such an annoyance. But that’s to be expected of the folk of this Gensōkyō.");

	MSG(youmu, "And I see that you’re a master of the blade, a shinigami no less!");
	MSG(youmu, "Absolutely splendid!");

	TITLE(elly, "Elly", "The Theoretical Reaper");
	MSG(elly, "Ah, I’m no shinigami, especially not now.");

	FACE(youmu, eyes_closed);
	MSG(youmu, "The scythe is a rather particular weapon, you must understand. Forgive me presumptiveness.");

	MSG(elly, "I see. I used to rely on this scythe as a gatekeeper, in the real Gensōkyō.");
	FACE(elly, angry);
	MSG(elly, "Now, it will the sigil of a new era!");
	FACE(elly, shouting);
	MSG(elly, "One that we, the ‘forgotten’, will reign over!");

	MSG(youmu, "Such an unwieldy weapon… for anyone to choose it willingly…");
	MSG(youmu, "Either you’re incredibly skilled, or incredibly foolish.");
	FACE(youmu, chuuni);
	MSG(youmu, "I wish to find that out!");
	MSG(youmu, "After our duel, let us meet and discuss our techniques!");

	FACE(elly, angry);
	MSG(elly, "Techniques? Hah!");
	MSG(elly, "Your existence means nothing to us! And I’m sure our existence means nothing to you, either!");

	FACE(youmu, chuuni);
	MSG(youmu, "On the contrary, scythe-bearer!");
	MSG(youmu, "You’ve created an impeccable stage for our final confrontation.");
	FACE(youmu, happy);
	MSG(youmu, "Truly, one of the best. I applaud you!");
	FACE(youmu, eyes_closed);
	MSG(youmu, "But first, please, tell me: what is your mission? What is it you desire?");
	FACE(youmu, chuuni);
	MSG(youmu, "For us to share this… intimate moment, I desire to know everything about you.");

	FACE(elly, blush);
	MSG(elly, "…!");
	MSG(elly, "O-Our mission, is t-to—");

	FACE(elly, eyes_closed);
	MSG(elly, "…");
	FACE(elly, angry);
	MSG(elly, "… is to avenge our forgotten f-friends!");
	MSG(elly, "We will create a new era of Gensōkyō, where the truly forgotten can live another day!");
	MSG(elly, "Soon, the residents of this land will be driven mad beyond comprehension, and be ripe to conquer!");
	FACE(youmu, eyes_closed);
	MSG(youmu, "So you wish to avenge your fallen compatriots… how noble…");
	MSG(youmu, "I can feel it move me to tears…");
	FACE(youmu, chuuni);
	MSG(youmu, "Alas, my mission, given to me by Lady Yuyuko of the Netherworld, directly conflicts with yours!");
	MSG(youmu, "I cannot allow you to drive my compatriots mad, or to subjugate my home!");
	FACE(youmu, eyes_closed);
	MSG(youmu, "Perhaps, in another time and place, we could have been friends!");

	FACE(elly, angry);
	MSG(elly, "B-Being friends with you heartless folk would be a sin! I could never!");

	FACE(youmu, chuuni);
	MSG(youmu, "What a tragedy!");

	MSG(elly, "You’re aiming to be the big ‘hero’ of this Gensōkyō, aren’t you?");

	MSG(youmu, "Naturally! No different than how you wish to be the hero of ‘your’ Gensōkyō!");
	MSG(youmu, "We’re not so different, you and I.");

	FACE(elly, smug);
	MSG(elly, "Your courage is admirable, I’ll give you that.");
	FACE(elly, angry);
	MSG(elly, "But there’s an enormous difference between us, swordswoman.");

	EVENT(music_changes);

	FACE(elly, shouting);
	FACE(youmu, normal);
	MSG(elly, "For I possess the knowledge of science!");
	MSG(elly, "In a magic-filled world like this, science and rationality cut through opponents with unparalleled precision!");
	MSG(elly, "Let’s see what good your swords do you when you fall prey to true enlightenment!");

	DIALOG_END();
}

DIALOG_TASK(youmu, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, angry);

	FACE(youmu, chuuni);
	MSG(youmu, "Impressive! You’ve pushed me to 10% of my true power!");

	MSG(elly, "Your despicable pride will be your downfall!");
	FACE(elly, shouting);
	MSG(elly, "Behold your judgement before the true nature of reality!");

	DIALOG_END();
}

/*
 * Extra Stage
 */

DIALOG_TASK(youmu, StageExPreBoss) {
	DIALOG_BEGIN(StageExPreBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(yumemi);

	EVENT(boss_appears);
	SHOW(yumemi);
	FACE(youmu, normal);
	MSG(youmu, "Greetings, weary adversary.");
	FACE(yumemi, surprised);
	MSG(yumemi, "‘Weary’?");
	FACE(youmu, eyes_closed);
	MSG(youmu, "‘The slings and arrows of outrageous fortune’, perhaps?");
	FACE(youmu, smug);
	MSG(youmu, "Given the ostentatiousness of this contraption we find ourselves in.");

	MSG(yumemi, "Outrageous fortune? Perish the thought.");
	FACE(yumemi, normal);
	MSG(yumemi, "Hmm…");
	FACE(yumemi, surprised);
	MSG(yumemi, "… so the swordswoman has actual katanas…");
	FACE(yumemi, smug);
	FACE(youmu, normal);
	MSG(yumemi, "Hilarious.");
	FACE(yumemi, normal);
	MSG(yumemi, "In my time, those swords of yours were banned over two hundred years ago.");
	MSG(yumemi, "The great machine of war became more detached and brutal ever since the Industrial Revolution. Nobody has any use for them.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Just another relic of human history.");

	FACE(youmu, eyes_closed);
	MSG(youmu, "Yes, I am aware of the Outside World’s current martial doctrines.");
	FACE(youmu, chuuni);
	MSG(youmu, "And I’m glad to hear that you agree, that these are more… elegant weapons, for a more honourable age.");

	FACE(yumemi, surprised);
	MSG(yumemi, "…");
	MSG(yumemi, "(First Shakespeare, and then…)");
	FACE(yumemi, normal);
	MSG(yumemi, "Your martial art is a pastime of the wealthy.");
	FACE(youmu, normal);
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "But of course, I’m sure many people fruitlessly ‘train’ in virtual reality with those weapons, for absolutely no purpose other than delusions of grandeur.");

	MSG(youmu, "Fine blades such as these are hard to come by for the working class, it’s true.");
	FACE(youmu, chuuni);
	MSG(youmu, "But I am privileged enough to be in the service of Lady Yuyuko, who graciously provides—");

	FACE(youmu, normal);
	FACE(yumemi, sigh);
	MSG(yumemi, "Please, stop. I don’t care about any of that. It’s meaningless.");

	FACE(youmu, unamused);
	MSG(youmu, "Hmph. I was warned of your attitude.");
	MSG(youmu, "And what I’ve heard is evidently true. You care for nothing and no one.");
	FACE(youmu, sigh);
	MSG(youmu, "A hollow husk of a woman, even compared to me.");

	FACE(yumemi, sad);
	MSG(yumemi, "Excuse me?");

	FACE(youmu, normal);
	MSG(youmu, "Your expressions, body language, words…");
	MSG(youmu, "Yes, you’ve lost the spark which gives one meaning in life.");

	FACE(yumemi, normal);
	MSG(yumemi, "You know nothing about me.");
	MSG(yumemi, "You exist in an unreal world, living a false life, surrounded by simple and childish delusions about your powers and abilities.");

	FACE(youmu, smug);
	MSG(youmu, "As opposed to whom? You?");

	FACE(yumemi, surprised);
	MSG(yumemi, "Y—…");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Yes. Me.");

	FACE(youmu, eyes_closed);
	MSG(youmu, "I see.");

	FACE(youmu, smug);
	FACE(yumemi, sigh);
	MSG(yumemi, "I don’t have the patience to take criticism from the likes of you. I’ll destroy this delusion and be on my way.");

	FACE(youmu, puzzled);
	MSG(youmu, "How odd. Have the denizens of Gensōkyō given you cause for retribution?");

	FACE(yumemi, normal);
	FACE(youmu, normal);
	MSG(yumemi, "That remains to be seen.");

	FACE(youmu, puzzled);
	MSG(youmu, "Excuse me?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "It could be this Gensōkyō, or some other Gensōkyō…");
	FACE(youmu, normal);
	FACE(yumemi, sigh);
	MSG(yumemi, "To be honest, I can’t tell the difference anymore.");

	MSG(youmu, "Some… other Gensōkyō?");

	FACE(yumemi, normal);
	MSG(yumemi, "All of them have a common effect on the real world.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "They lead people astray, into forgetting about…");
	FACE(youmu, normal);
	FACE(yumemi, sad);
	MSG(yumemi, "… ah, I just don’t care anymore.");
	MSG(yumemi, "It probably won’t change a thing.");
	FACE(yumemi, sigh);
	MSG(yumemi, "Perhaps I just wanted to see if this contraption worked, in the end.");
	FACE(yumemi, normal);
	MSG(yumemi, "It’s not worth explaining anything else.");

	FACE(youmu, unamused);
	MSG(youmu, "…");
	MSG(youmu, "Is that… it?");

	FACE(yumemi, surprised);
	MSG(yumemi, "What else were you expecting? A grand vision for the future?");
	FACE(yumemi, sigh);
	MSG(yumemi, "Please, I’m past all that.");

	MSG(youmu, "… pathetic!");

	FACE(yumemi, surprised);
	MSG(yumemi, "Excuse me?");

	MSG(youmu, "Elly had a purpose, a passion! To save her friends, and to restore her world!");
	MSG(youmu, "But you…! You just want an apocalypse! Over something you no longer care for?!");

	FACE(yumemi, sad);
	MSG(yumemi, "I don’t have anything *left* to care for, so what does it matter?");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "It’s not as if you’re real. You’re a fairytale out of a children’s book.");

	MSG(youmu, "And what could you have possibly lost that would explain this casually genocidal attitude?");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Isn’t it obvious by now?");
	FACE(yumemi, sad);
	MSG(yumemi, "Gensōkyō stole away the only person I ever loved.");
	MSG(yumemi, "And I don’t care which Gensōkyō did it. Not anymore.");

	FACE(youmu, eyes_closed);
	MSG(youmu, "…");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "…");

	MSG(youmu, "Spirited away, hmm?");
	FACE(youmu, unamused);
	MSG(youmu, "And this is how you choose to honour their memory? I see.");

	FACE(youmu, normal);
	FACE(yumemi, sigh);
	MSG(yumemi, "No, you don’t. You’re a figment of a weary society’s delusional imagination.");
	EVENT(music_changes);
	MSG(yumemi, "I’m tired of explaining myself to the likes of you.");
	FACE(yumemi, smug);
	MSG(yumemi, "Let me demonstrate the full power of this technological terror I’ve constructed.");

	DIALOG_END();
}

DIALOG_TASK(youmu, StageExPostBoss) {
	DIALOG_BEGIN(StageExPostBoss);

	ACTOR_LEFT(youmu);
	ACTOR_RIGHT(yumemi);
	VARIANT(yumemi, defeated);
	FACE(yumemi, defeated);

	FACE(yumemi, sad);
	MSG(yumemi, "I yield.");
	MSG(yumemi, "To think I’ll meet my end to someone like you…");

	FACE(youmu, eyes_closed);
	MSG(youmu, "An academic such as yourself ought to know that martial arts are a field of study, like any other.");
	FACE(youmu, chuuni);
	MSG(youmu, "You lost to a subject matter expert, nothing more.");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "… ugh…");

	FACE(youmu, eyes_closed);
	MSG(youmu, "With as empty as your heart is, you might be unaware of the gift you’ve given me, weary traveller.");

	FACE(yumemi, surprised);
	MSG(yumemi, "Gift…? And what’s that…?");

	MSG(youmu, "Before this incident, I was terrified of my own existence.");
	MSG(youmu, "Between worlds, perpetually troubled by my mortality and immortality…");
	MSG(youmu, "… my human side, and my phantom side.");
	FACE(yumemi, normal);
	FACE(youmu, normal);
	MSG(youmu, "When I came to this infernal place, something changed within my heart, mind, and soul.");
	FACE(youmu, chuuni);
	MSG(youmu, "Suddenly, I was no longer frightened of my true nature! My fear burned away!");
	MSG(youmu, "I recognized my talents and beauty as a warrior, fully and earnestly!");
	MSG(youmu, "The ‘destructive madness’ you sought to destroy me with? It only made me stronger!");
	MSG(youmu, "And it has persisted even with your audacious machine deactivated!");

	FACE(yumemi, surprised);
	MSG(yumemi, "Am I hearing you clearly…? Instead of realizing your unreal nature, you became permanently more confident in yourself?");
	MSG(youmu, "Indeed. I was as startled as you are at first.");

	FACE(youmu, normal);
	FACE(yumemi, smug);
	MSG(yumemi, "Hah. Hahaha. Incredible.");

	FACE(youmu, smug);
	MSG(youmu, "Empty-hearted retribution will never defeat the will of those who believe in themselves.");
	FACE(youmu, chuuni);
	FACE(yumemi, normal);
	MSG(youmu, "The outcome of this battle was—");

	WAIT(60);
	ACTOR_LEFT(elly);
	MSG(elly, "I’m here, Yōmu! I couldn’t stand the idea of you having to face my mistake alone, and—");
	MSG(elly, "…?!");
	MSG(elly, "Y-you already defeated her?!");

	FACE(youmu, smug);
	MSG(youmu, "Fear not, I’ve resolved the situation.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "Her techniques were powerful and precise, but as I was just saying…");

	FACE(yumemi, sad);
	MSG(yumemi, "…");
	MSG(yumemi, "‘The outcome of this battle was determined before it began’?");

	FACE(youmu, smug);
	MSG(youmu, "Precisely.");
	FACE(elly, blush);
	MSG(elly, "Yōmu, you’re absolutely incredible!");
	FACE(youmu, chuuni);
	MSG(youmu, "Flattery will get you everywhere, my dear.");
	MSG(yumemi, "After all that effort you put into stealing the Tower from me…");
	FACE(youmu, normal);
	FACE(elly, normal);
	MSG(yumemi, "… that’s all it took for you to give up on your vision, Elly? A delusional katana-wielding woman with cheesy action-hero one-liners?");
	MSG(yumemi, "I never did stand a chance, did I?");

	FACE(elly, angry);
	MSG(elly, "No, I suppose you didn’t.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Hah…");
	MSG(yumemi, "Sentence me as you wish. I won’t resist.");

	FACE(youmu, eyes_closed);
	FACE(elly, normal);
	MSG(youmu, "Very well.");
	FACE(youmu, normal);
	MSG(youmu, "Allowing you to leave is out of the question. The Tower may continue to drive you mad, and you may come back, with reinforcements.");
	MSG(youmu, "As such, I would order you to power down the tower, and agree to live here in Gensōkyō peacefully. However…");
	FACE(youmu, eyes_closed);
	MSG(youmu, "As Elly is one of the most impacted victims, I must ask for her input.");

	FACE(elly, angry);
	MSG(elly, "…");
	FACE(elly, normal);
	MSG(elly, "…");
	MSG(elly, "I know her pain. She’s suffered enough.");
	MSG(elly, "I’ll agree to whatever you decide, Yōmu.");

	FACE(youmu, normal);
	MSG(youmu, "Very well, then.");
	FACE(youmu, eyes_closed);
	MSG(youmu, "And so, those are the terms. Will you abide by them, weary traveller?");

	FACE(yumemi, sad);
	MSG(yumemi, "… you’re not going to…?");

	FACE(youmu, eyes_closed);
	MSG(youmu, "Take your life? Perish the thought.");
	FACE(youmu, normal);
	MSG(youmu, "A human life is too precious and fleeting for such violence.");

	FACE(yumemi, sad);
	MSG(yumemi, "I… see.");
	MSG(yumemi, "Then, I agree. I’ll turn it all off.");
	FACE(yumemi, normal);
	MSG(yumemi, "As I’ll be here for a while, I figure I may as well ask something, if you’ll indulge me.");

	FACE(youmu, puzzled);
	MSG(youmu, "What is it?");

	MSG(yumemi, "Is there someone here… who can manipulate the boundaries of reality?");

	FACE(youmu, eeeeh);
	MSG(youmu, "…?!");
	FACE(youmu, eyes_closed);
	MSG(youmu, "… ahem. Yes, there is. I’m well-acquainted with her, oddly enough.");

	FACE(yumemi, surprised);
	MSG(yumemi, "…?!");

	MSG(elly, "Are you talking about that blonde lady, the one always visiting Lady Yuyuko?");
	MSG(elly, "What was her name again?");
	MSG(elly, "The eyes she has in her ‘gaps’ give me the creeps…");

	FACE(youmu, eeeeh);
	MSG(youmu, "S-she is frightening to most people in Gensōkyō, myself included, even in my… improved state.");
	FACE(youmu, puzzled);
	MSG(youmu, "Does that appear to be the one you seek?");

	MSG(yumemi, "I’m not sure. Maybe…");

	FACE(youmu, eyes_closed);
	MSG(youmu, "Perhaps Lady Yuyuko will grace you with her legendary generosity, and grant you an audience with her… companion.");

	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Thank you. That’s more than I can ask.");

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER youmu
#include "export_dialog_tasks.inc.h"
