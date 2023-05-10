#!/bin/bash
tx pull --all --force
pushd translations

mv shotcut_cs_CZ.ts shotcut_cs.ts
mv shotcut_da_DK.ts shotcut_da.ts
mv shotcut_el_GR.ts shotcut_el.ts
mv shotcut_hu_HU.ts shotcut_hu.ts
mv shotcut_it_IT.ts shotcut_it.ts
mv shotcut_ku_IQ.ts shotcut_ku.ts
mv shotcut_nl_NL.ts shotcut_nl.ts
mv shotcut_tr_TR.ts shotcut_tr.ts
popd

git restore translations/shotcut_en.ts

