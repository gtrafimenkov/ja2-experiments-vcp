// #include "Tactical/QArray.h"
//
// BOOLEAN QuoteExp_HeadShotOnly[75] = {
//     // this is for quote 33 (0=generic grossout, 1=headshot specific)
//     /* 0	Barry  	*/ 1,
//     /* 1	Blood  	*/ 1,
//     /* 2	Lynx	*/ 1,
//     /* 3	Grizzly	*/ 1,
//     /* 4	Vicki  	*/ 1,
//     /* 5	Trevor	*/ 0,
//     /* 6	Grunty	*/ 1,
//     /* 7	Ivan	*/ 0,
//     /* 8	Steroid	*/ 1,
//     /* 9	Igor	*/ 0,
//     /* 10	Shadow	*/ 0,
//     /* 11	Red	*/ 0,
//     /* 12	Reaper	*/ 1,
//     /* 13	Fidel	*/ 1,
//     /* 14	Fox	*/ 0,
//     /* 15	Sidney	*/ 1,
//     /* 16	Gus	*/ 1,
//     /* 17	Buns	*/ 0,
//     /* 18	Ice	*/ 1,
//     /* 19	Spider	*/ 0,
//     /* 20	Cliff	*/ 1,
//     /* 21	Bull	*/ 1,
//     /* 22	Hitman	*/ 1,
//     /* 23	Buzz	*/ 1,
//     /* 24	Raider	*/ 0,
//     /* 25	Raven	*/ 1,
//     /* 26	Static	*/ 1,
//     /* 27	Len	*/ 1,
//     /* 28	Danny	*/ 1,
//     /* 29	Magic	*/ 1,
//     /* 30	Stephan	*/ 0,
//     /* 31	Scully	*/ 1,
//     /* 32	Malice	*/ 1,
//     /* 33	Dr. Q	*/ 1,
//     /* 34	Nails	*/ 1,
//     /* 35	Thor	*/ 1,
//     /* 36	Scope	*/ 1,
//     /* 37	Wolf	*/ 0,
//     /* 38	MD	*/ 0,
//     /* 39	Meltdown*/ 0,
//     /* 40	Biff	*/ 1,
//     /* 41	Haywire	*/ 1,
//     /* 42	Gasket	*/ 1,
//     /* 43	Razor	*/ 1,
//     /* 44	Flo	*/ 0,
//     /* 45	Gumpy	*/ 1,
//     /* 46	Larryok	*/ 0,
//     /* 47	Larryold*/ 0,
//     /* 48	Cougar	*/ 0,
//     /* 49	Numb	*/ 1,
//     /* 50	Bubba	*/ 1,
//     /* 51	PGmale1	*/ 1,
//     /* 52	PGmale2	*/ 1,
//     /* 53	PGmale3	*/ 1,
//     /* 54	PGLady1	*/ 0,
//     /* 55	PGLady2	*/ 0,
//     /* 56	PGLady3	*/ 0,
//
//     /* 57	Miguel	*/ 0,  // these RPC's don't have quotes written yet
//     /* 58	Carlos	*/ 0,  // so this data is temporary.
//     /* 59	Ira	*/ 0,
//     /* 60	Dimitri	*/ 0,
//     /* 61	Devin	*/ 0,
//     /* 62	Rat	*/ 0,
//     /* 63	Madlab	*/ 0,
//     /* 64	Slay	*/ 0,
//     /* 65	Nanchuk	*/ 0,
//     /* 66	Dynamo	*/ 0,
//     /* 67	Prisoner*/ 0,
//     /* 68	Iggy	*/ 0,
//     /* 69	Doctor	*/ 0,
//     /* 70	MTI	*/ 0,
//     /* 71	RPC71	*/ 0,
//     /* 72	Junkson	*/ 0,
//     /* 73	RPC73	*/ 0,
//     /* 74	RPC74	*/ 0};
//
// BOOLEAN QuoteExp_TeamSpecific[75] = {
//     // this is for quote 41 (0=refers to anyone, 1=team specific)
//     /* 0	Barry  	*/ 1,
//     /* 1	Blood  	*/ 1,
//     /* 2	Lynx	*/ 1,
//     /* 3	Grizzly	*/ 1,
//     /* 4	Vicki  	*/ 1,
//     /* 5	Trevor	*/ 1,
//     /* 6	Grunty	*/ 1,
//     /* 7	Ivan	*/ 0,
//     /* 8	Steroid	*/ 1,
//     /* 9	Igor	*/ 1,
//     /* 10	Shadow	*/ 1,
//     /* 11	Red	*/ 0,
//     /* 12	Reaper	*/ 1,
//     /* 13	Fidel	*/ 1,
//     /* 14	Fox	*/ 1,
//     /* 15	Sidney	*/ 1,
//     /* 16	Gus	*/ 0,
//     /* 17	Buns	*/ 1,
//     /* 18	Ice	*/ 1,
//     /* 19	Spider	*/ 1,
//     /* 20	Cliff	*/ 1,
//     /* 21	Bull	*/ 1,
//     /* 22	Hitman	*/ 1,
//     /* 23	Buzz	*/ 1,
//     /* 24	Raider	*/ 1,
//     /* 25	Raven	*/ 1,
//     /* 26	Static	*/ 1,
//     /* 27	Len	*/ 1,
//     /* 28	Danny	*/ 0,
//     /* 29	Magic	*/ 1,
//     /* 30	Stephan	*/ 1,
//     /* 31	Scully	*/ 1,
//     /* 32	Malice	*/ 0,
//     /* 33	Dr. Q	*/ 1,
//     /* 34	Nails	*/ 1,
//     /* 35	Thor	*/ 1,
//     /* 36	Scope	*/ 1,
//     /* 37	Wolf	*/ 0,
//     /* 38	MD	*/ 1,
//     /* 39	Meltdown*/ 0,
//     /* 40	Biff	*/ 1,
//     /* 41	Haywire	*/ 1,
//     /* 42	Gasket	*/ 1,
//     /* 43	Razor	*/ 1,
//     /* 44	Flo	*/ 0,
//     /* 45	Gumpy	*/ 1,
//     /* 46	Larryok	*/ 1,
//     /* 47	Larryold*/ 0,
//     /* 48	Cougar	*/ 1,
//     /* 49	Numb	*/ 1,
//     /* 50	Bubba	*/ 0,
//     /* 51	PGmale1	*/ 1,
//     /* 52	PGmale2	*/ 1,
//     /* 53	PGmale3	*/ 1,
//     /* 54	PGLady1	*/ 1,
//     /* 55	PGLady2	*/ 1,
//     /* 56	PGLady3	*/ 1,
//
//     /* 57	Miguel	*/ 0,  // these RPC's don't have quotes written yet
//     /* 58	Carlos	*/ 0,  // so this data is temporary.
//     /* 59	Ira	*/ 0,
//     /* 60	Dimitri	*/ 0,
//     /* 61	Devin	*/ 0,
//     /* 62	Rat	*/ 0,
//     /* 63	Madlab	*/ 0,
//     /* 64	Slay	*/ 0,
//     /* 65	Nanchuk	*/ 0,
//     /* 66	Dynamo	*/ 0,
//     /* 67	Prisoner*/ 0,
//     /* 68	Iggy	*/ 0,
//     /* 69	Doctor	*/ 0,
//     /* 70	MTI	*/ 0,
//     /* 71	RPC71	*/ 0,
//     /* 72	Junkson	*/ 0,
//     /* 73	RPC73	*/ 0,
//     /* 74	RPC74	*/ 0};
//
// BOOLEAN QuoteExp_GenderCode[75] = {
//     // this is for quote 58 (0=male, 1=female, 2=either)
//     /* 0	Barry  	*/ 0,
//     /* 1	Blood  	*/ 2,
//     /* 2	Lynx	*/ 0,
//     /* 3	Grizzly	*/ 2,
//     /* 4	Vicki  	*/ 2,
//     /* 5	Trevor	*/ 2,
//     /* 6	Grunty	*/ 2,
//     /* 7	Ivan	*/ 2,
//     /* 8	Steroid	*/ 0,
//     /* 9	Igor	*/ 2,
//     /* 10	Shadow	*/ 0,
//     /* 11	Red	*/ 1,
//     /* 12	Reaper	*/ 0,
//     /* 13	Fidel	*/ 2,
//     /* 14	Fox	*/ 0,
//     /* 15	Sidney	*/ 0,
//     /* 16	Gus	*/ 1,
//     /* 17	Buns	*/ 2,
//     /* 18	Ice	*/ 0,
//     /* 19	Spider	*/ 1,
//     /* 20	Cliff	*/ 0,
//     /* 21	Bull	*/ 2,
//     /* 22	Hitman	*/ 2,
//     /* 23	Buzz	*/ 0,
//     /* 24	Raider	*/ 2,
//     /* 25	Raven	*/ 2,
//     /* 26	Static	*/ 2,
//     /* 27	Len	*/ 2,
//     /* 28	Danny	*/ 0,
//     /* 29	Magic	*/ 2,
//     /* 30	Stephan	*/ 0,
//     /* 31	Scully	*/ 2,
//     /* 32	Malice	*/ 2,
//     /* 33	Dr. Q	*/ 0,
//     /* 34	Nails	*/ 2,
//     /* 35	Thor	*/ 2,
//     /* 36	Scope	*/ 2,
//     /* 37	Wolf	*/ 2,
//     /* 38	MD	*/ 2,
//     /* 39	Meltdown*/ 2,
//     /* 40	Biff	*/ 2,
//     /* 41	Haywire	*/ 0,
//     /* 42	Gasket	*/ 2,
//     /* 43	Razor	*/ 0,
//     /* 44	Flo	*/ 2,
//     /* 45	Gumpy	*/ 0,
//     /* 46	Larryok	*/ 1,
//     /* 47	Larryold*/ 2,
//     /* 48	Cougar	*/ 2,
//     /* 49	Numb	*/ 0,
//     /* 50	Bubba	*/ 1,
//     /* 51	PGmale1	*/ 0,
//     /* 52	PGmale2	*/ 2,
//     /* 53	PGmale3	*/ 2,
//     /* 54	PGLady1	*/ 0,
//     /* 55	PGLady2	*/ 1,
//     /* 56	PGLady3	*/ 2,
//
//     /* 57	Miguel	*/ 0,  // these RPC's don't have quotes written yet
//     /* 58	Carlos	*/ 0,  // so this data is temporary.
//     /* 59	Ira	*/ 0,
//     /* 60	Dimitri	*/ 0,
//     /* 61	Devin	*/ 0,
//     /* 62	Rat	*/ 0,
//     /* 63	Madlab	*/ 0,
//     /* 64	Slay	*/ 0,
//     /* 65	Nanchuk	*/ 0,
//     /* 66	Dynamo	*/ 0,
//     /* 67	Prisoner*/ 0,
//     /* 68	Iggy	*/ 0,
//     /* 69	Doctor	*/ 0,
//     /* 70	MTI	*/ 0,
//     /* 71	RPC71	*/ 0,
//     /* 72	Junkson	*/ 0,
//     /* 73	RPC73	*/ 0,
//     /* 74	RPC74	*/ 0};
//
// BOOLEAN QuoteExp_GotGunOrUsedGun[75] = {
//     // this is to indicate whether they have quote 61 or 62
//     /* 0	Barry	*/ 62,
//     /* 1	Blood   */ 62,
//     /* 2	Lynx	*/ 62,
//     /* 3	Grizzly */ 62,
//     /* 4	Vicki   */ 61,
//     /* 5	Trevor  */ 61,
//     /* 6	Grunty  */ 61,
//     /* 7	Ivan    */ 61,
//     /* 8	Steroid */ 61,
//     /* 9	Igor    */ 62,
//     /* 10	Shadow  */ 62,
//     /* 11	Red     */ 61,
//     /* 12	Reaper  */ 61,
//     /* 13	Fidel   */ 62,
//     /* 14	Fox     */ 61,
//     /* 15	Sidney  */ 62,
//     /* 16	Gus     */ 62,
//     /* 17	Buns    */ 62,
//     /* 18	Ice     */ 62,
//     /* 19	Spider  */ 61,
//     /* 20	Cliff   */ 62,
//     /* 21	Bull    */ 62,
//     /* 22	Hitman  */ 61,
//     /* 23	Buzz    */ 62,
//     /* 24	Raider  */ 62,
//     /* 25	Raven   */ 62,
//     /* 26	Static  */ 62,
//     /* 27	Len     */ 62,
//     /* 28	Danny   */ 61,
//     /* 29	Magic   */ 62,
//     /* 30	Stephan */ 62,
//     /* 31	Scully  */ 62,
//     /* 32	Malice  */ 62,
//     /* 33	Dr. Q   */ 62,
//     /* 34	Nails   */ 62,
//     /* 35	Thor    */ 61,
//     /* 36	Scope   */ 61,
//     /* 37	Wolf    */ 62,
//     /* 38	MD      */ 61,
//     /* 39	Meltdown*/ 62,
//     /* 40	Biff    */ 61,
//     /* 41	Haywire */ 62,
//     /* 42	Gasket  */ 61,
//     /* 43	Razor   */ 61,
//     /* 44	Flo     */ 62,
//     /* 45	Gumpy   */ 62,
//     /* 46	Larryok */ 61,
//     /* 47	Larryold*/ 61,
//     /* 48	Cougar  */ 62,
//     /* 49	Numb    */ 61,
//     /* 50	Bubba   */ 62,
//     /* 51	PGmale1 */ 62,
//     /* 52	PGmale2 */ 62,
//     /* 53	PGmale3 */ 61,
//     /* 54	PGLady1 */ 61,
//     /* 55	PGLady2 */ 61,
//     /* 56	PGLady3 */ 62,
//
//     /* 57	Miguel  */ 62,
//     /* 58	Carlos  */ 61,
//     /* 59	Ira     */ 62,
//     /* 60	Dimitri */ 61,
//     /* 61	Devin   */ 0,
//     /* 62	Rat     */ 0,
//     /* 63	Madlab  */ 0,
//     /* 64	Slay    */ 0,
//     /* 65	Nanchuk */ 0,
//     /* 66	Dynamo  */ 62,
//     /* 67	Shank   */ 61,
//     /* 68	Iggy    */ 0,
//     /* 69	Doctor  */ 0,
//     /* 70	MTI     */ 0,
//     /* 71	RPC71   */ 0,
//     /* 72	Junkson */ 0,
//     /* 73	RPC73   */ 0,
//     /* 74	RPC74   */ 0};
//
// BOOLEAN QuoteExp_PassingDislike[75] = {
//     // this is to indicate what kind of quote 45 (if any) mercs have
//     /* 0	Barry	*/ 2,
//     /* 1	Blood   */ 4,
//     /* 2	Lynx	*/ 2,
//     /* 3	Grizzly */ 5,
//     /* 4	Vicki   */ 2,
//     /* 5	Trevor  */ 4,
//     /* 6	Grunty  */ 2,
//     /* 7	Ivan    */ 0,
//     /* 8	Steroid */ 4,
//     /* 9	Igor    */ 0,
//     /* 10	Shadow  */ 4,
//     /* 11	Red     */ 4,
//     /* 12	Reaper  */ 0,
//     /* 13	Fidel   */ 4,
//     /* 14	Fox     */ 3,
//     /* 15	Sidney  */ 4,
//     /* 16	Gus     */ 4,
//     /* 17	Buns    */ 3,
//     /* 18	Ice     */ 4,
//     /* 19	Spider  */ 2,
//     /* 20	Cliff   */ 5,
//     /* 21	Bull    */ 0,
//     /* 22	Hitman  */ 0,
//     /* 23	Buzz    */ 6,
//     /* 24	Raider  */ 0,
//     /* 25	Raven   */ 0,
//     /* 26	Static  */ 4,
//     /* 27	Len     */ 2,
//     /* 28	Danny   */ 2,
//     /* 29	Magic   */ 4,
//     /* 30	Stephan */ 4,
//     /* 31	Scully  */ 1,
//     /* 32	Malice  */ 4,
//     /* 33	Dr. Q   */ 0,
//     /* 34	Nails   */ 1,
//     /* 35	Thor    */ 0,
//     /* 36	Scope   */ 4,
//     /* 37	Wolf    */ 0,
//     /* 38	MD      */ 0,
//     /* 39	Meltdown*/ 2,
//     /* 40	Biff    */ 0,
//     /* 41	Haywire */ 5,
//     /* 42	Gasket  */ 4,
//     /* 43	Razor   */ 4,
//     /* 44	Flo     */ 0,
//     /* 45	Gumpy   */ 0,
//     /* 46	Larryok */ 0,
//     /* 47	Larryold*/ 0,
//     /* 48	Cougar  */ 0,
//     /* 49	Numb    */ 4,
//     /* 50	Bubba   */ 4,
//     /* 51	PGmale1 */ 0,
//     /* 52	PGmale2 */ 0,
//     /* 53	PGmale3 */ 0,
//     /* 54	PGLady1 */ 0,
//     /* 55	PGLady2 */ 0,
//     /* 56	PGLady3 */ 0,
//
//     /* 57	Miguel  */ 4,
//     /* 58	Carlos  */ 2,
//     /* 59	Ira     */ 4,
//     /* 60	Dimitri */ 0,
//     /* 61	Devin   */ 0,
//     /* 62	RPC62   */ 0,
//     /* 63	Madlab  */ 0,
//     /* 64	Slay    */ 0,
//     /* 65	RPC65   */ 0,
//     /* 66	Dynamo  */ 4,
//     /* 67	Shank   */ 0,
//     /* 68	Iggy    */ 0,
//     /* 69	Steve   */ 0,
//     /* 70	MTI     */ 0,
//     /* 71	RPC71   */ 0,
//     /* 72	Maddog  */ 0,
//     /* 73	RPC73   */ 0,
//     /* 74	RPC74   */ 0};
//
// BOOLEAN QuoteExp_WitnessDeidrannaDeath[73] = {
//     // this is to indicate what kind of quote 37,
//     // (Killed Deidranna) quote mercs have, 0 means
//     // it can only be used if merc is the KILLER, a
//     // 1 means it can be used if they have LOS to
//     // her at time of her death.
//     /* 0	Barry	*/ 0,
//     /* 1	Blood   */ 0,
//     /* 2	Lynx	*/ 0,
//     /* 3	Grizzly */ 1,
//     /* 4	Vicki   */ 1,
//     /* 5	Trevor  */ 1,
//     /* 6	Grunty  */ 0,
//     /* 7	Ivan    */ 0,
//     /* 8	Steroid */ 0,
//     /* 9	Igor    */ 1,
//     /* 10	Shadow  */ 1,
//     /* 11	Red     */ 0,
//     /* 12	Reaper  */ 1,
//     /* 13	Fidel   */ 0,
//     /* 14	Fox     */ 1,
//     /* 15	Sidney  */ 1,
//     /* 16	Gus     */ 1,
//     /* 17	Buns    */ 1,
//     /* 18	Ice     */ 1,
//     /* 19	Spider  */ 1,
//     /* 20	Cliff   */ 0,
//     /* 21	Bull    */ 0,
//     /* 22	Hitman  */ 1,
//     /* 23	Buzz    */ 0,
//     /* 24	Raider  */ 1,
//     /* 25	Raven   */ 0,
//     /* 26	Static  */ 1,
//     /* 27	Len     */ 1,
//     /* 28	Danny   */ 1,
//     /* 29	Magic   */ 1,
//     /* 30	Stephan */ 1,
//     /* 31	Scully  */ 1,
//     /* 32	Malice  */ 1,
//     /* 33	Dr. Q   */ 1,
//     /* 34	Nails   */ 1,
//     /* 35	Thor    */ 1,
//     /* 36	Scope   */ 1,
//     /* 37	Wolf    */ 0,
//     /* 38	MD      */ 0,
//     /* 39	Meltdown*/ 1,
//     /* 40	Biff    */ 1,
//     /* 41	Haywire */ 1,
//     /* 42	Gasket  */ 1,
//     /* 43	Razor   */ 1,
//     /* 44	Flo     */ 0,
//     /* 45	Gumpy   */ 1,
//     /* 46	Larryok */ 0,
//     /* 47	Larryold*/ 1,
//     /* 48	Cougar  */ 1,
//     /* 49	Numb    */ 0,
//     /* 50	Bubba   */ 1,
//     /* 51	PGmale1 */ 1,
//     /* 52	PGmale2 */ 1,
//     /* 53	PGmale3 */ 1,
//     /* 54	PGLady1 */ 1,
//     /* 55	PGLady2 */ 0,
//     /* 56	PGLady3 */ 0,
//
//     /* 57	Miguel  */ 1,
//     /* 58	Carlos  */ 1,
//     /* 59	Ira     */ 1,
//     /* 60	Dimitri */ 0,
//     /* 61	Devin   */ 0,
//     /* 62	RPC62   */ 0,
//     /* 63	Madlab  */ 0,
//     /* 64	Slay    */ 0,
//     /* 65	RPC65   */ 0,
//     /* 66	Dynamo  */ 0,
//     /* 67	Shank   */ 0,
//     /* 68	Iggy    */ 0,
//     /* 69	Vince   */ 1,
//     /* 70	Conrad  */ 1,
//     /* 71	unused  */ 0,
//     /* 72	Maddog  */ 0,
// };
//
// BOOLEAN QuoteExp_WitnessQueenBugDeath[73] = {
//     // this is to indicate what kind of quote 38,
//     // (Killed Queen Bug) quote mercs have, 0 means
//     // it can only be used if merc is the KILLER, a
//     // 1 means it can be used if they have LOS to
//     // her at time of her death.
//     /* 0	Barry	*/ 0,
//     /* 1	Blood   */ 1,
//     /* 2	Lynx	*/ 1,
//     /* 3	Grizzly */ 1,
//     /* 4	Vicki   */ 1,
//     /* 5	Trevor  */ 1,
//     /* 6	Grunty  */ 0,
//     /* 7	Ivan    */ 1,
//     /* 8	Steroid */ 0,
//     /* 9	Igor    */ 1,
//     /* 10	Shadow  */ 1,
//     /* 11	Red     */ 0,
//     /* 12	Reaper  */ 1,
//     /* 13	Fidel   */ 0,
//     /* 14	Fox     */ 1,
//     /* 15	Sidney  */ 1,
//     /* 16	Gus     */ 1,
//     /* 17	Buns    */ 1,
//     /* 18	Ice     */ 1,
//     /* 19	Spider  */ 1,
//     /* 20	Cliff   */ 1,
//     /* 21	Bull    */ 1,
//     /* 22	Hitman  */ 1,
//     /* 23	Buzz    */ 1,
//     /* 24	Raider  */ 1,
//     /* 25	Raven   */ 1,
//     /* 26	Static  */ 1,
//     /* 27	Len     */ 1,
//     /* 28	Danny   */ 1,
//     /* 29	Magic   */ 1,
//     /* 30	Stephan */ 1,
//     /* 31	Scully  */ 1,
//     /* 32	Malice  */ 1,
//     /* 33	Dr. Q   */ 0,
//     /* 34	Nails   */ 1,
//     /* 35	Thor    */ 1,
//     /* 36	Scope   */ 1,
//     /* 37	Wolf    */ 1,
//     /* 38	MD      */ 0,
//     /* 39	Meltdown*/ 1,
//     /* 40	Biff    */ 0,
//     /* 41	Haywire */ 1,
//     /* 42	Gasket  */ 0,
//     /* 43	Razor   */ 1,
//     /* 44	Flo     */ 1,
//     /* 45	Gumpy   */ 0,
//     /* 46	Larryok */ 1,
//     /* 47	Larryold*/ 0,
//     /* 48	Cougar  */ 1,
//     /* 49	Numb    */ 0,
//     /* 50	Bubba   */ 0,
//     /* 51	PGmale1 */ 0,
//     /* 52	PGmale2 */ 0,
//     /* 53	PGmale3 */ 1,
//     /* 54	PGLady1 */ 1,
//     /* 55	PGLady2 */ 1,
//     /* 56	PGLady3 */ 0,
//
//     /* 57	Miguel  */ 1,
//     /* 58	Carlos  */ 1,
//     /* 59	Ira     */ 1,
//     /* 60	Dimitri */ 0,
//     /* 61	Devin   */ 1,
//     /* 62	RPC62   */ 0,
//     /* 63	Madlab  */ 0,
//     /* 64	Slay    */ 1,
//     /* 65	RPC65   */ 0,
//     /* 66	Dynamo  */ 0,
//     /* 67	Shank   */ 0,
//     /* 68	Iggy    */ 0,
//     /* 69	Vince   */ 1,
//     /* 70	Conrad  */ 1,
//     /* 71	unused  */ 0,
//     /* 72	Maddog  */ 1,
// };
//