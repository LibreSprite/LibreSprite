#!/usr/bin/env node
/**
 * The container runs Duktape (ES5.1), not V8 -- see src/script/CMakeLists.txt:21
 * and CMakeLists.txt:204-207. A single arrow function or `const` in a template
 * fails at runtime inside the container, where the error is hard to read.
 * Catch it here instead.
 *
 * Run after `npm run build`.
 */
import { parse } from 'acorn';

import { TEMPLATES, buildScript } from '../dist/templates.js';

// Enough shape for every template to render; only syntax is being checked.
const SAMPLE = {
  file: '/work/sample.ase',
  palette: '/work/master.gpl',
  layer: 'Base',
  frame: 0,
  ops: [],
  allowed: [],
};

let failed = 0;
for (const name of Object.keys(TEMPLATES)) {
  try {
    parse(buildScript(name, SAMPLE), { ecmaVersion: 5 });
    console.log(`  ok    ${name}`);
  } catch (err) {
    failed += 1;
    console.error(`  FAIL  ${name}: ${err.message}`);
  }
}

if (failed > 0) {
  console.error(`\n${failed} template(s) are not valid ES5.`);
  process.exit(1);
}
console.log(`\n${Object.keys(TEMPLATES).length} templates are valid ES5.`);
