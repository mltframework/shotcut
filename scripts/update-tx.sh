#!/bin/bash
tx pull --all --force
pushd translations

mv snapflow_cs_CZ.ts snapflow_cs.ts
mv snapflow_da_DK.ts snapflow_da.ts
mv snapflow_el_GR.ts snapflow_el.ts
mv snapflow_hu_HU.ts snapflow_hu.ts
mv snapflow_it_IT.ts snapflow_it.ts
mv snapflow_ku_IQ.ts snapflow_ku.ts
mv snapflow_nl_NL.ts snapflow_nl.ts
mv snapflow_tr_TR.ts snapflow_tr.ts
popd

git restore translations/snapflow_en.ts

