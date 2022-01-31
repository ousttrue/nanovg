from typing import Optional, List, Iterator, Union, Tuple, NamedTuple
import io
import logging
import pathlib
import sys
import re
logger = logging.getLogger(__name__)
MACRO = re.compile(r'#\s*(\w+)(.*)')


class Macro:
    def __init__(self, start: int, macro: str, arg: str) -> None:
        self.start = start+1
        self.macro = macro
        self.arg = arg
        self.lines: List[Union[str, 'Scope', 'Macro']] = []
        #
        self.end = -1

    def __str__(self) -> str:
        return f'[#{self.macro} {self.arg}, {self.start}: {self.end}]'

    def is_enable(self, definitions: List[str], any_prev: bool) -> bool:
        match self.macro:
            case 'if' | 'elif':
                if self.arg.startswith('defined '):
                    target = self.arg[8:].strip()
                    if target in definitions:
                        return True
                else:
                    target = self.arg.strip()
                    if target in definitions:
                        return True
                return False
            case 'ifdef':
                target = self.arg.strip()
                if target in definitions:
                    return True
                return False
            case 'ifndef':
                target = self.arg.strip()
                if target not in definitions:
                    return True
                return False
            case 'else':
                if any_prev:
                    return False
                return True
            case '':
                return True
            case _:
                raise NotImplementedError()


class Scope:
    def __init__(self, macro: Macro) -> None:
        self.macros: List[Macro] = [macro]

    def push(self, end: int, l: Union[str, 'Scope', 'Macro', None] = None):
        macro = self.macros[-1]
        macro.end = end + 1
        if l != None:
            macro.lines.append(l)

    def parse(self, it: Iterator[Tuple[int, str]]):
        # copy
        while True:
            try:
                i, l = next(it)
                m = MACRO.match(l)
                if m:
                    match m.group(1):
                        case 'ifdef':
                            child = Scope(Macro(i, m.group(1),
                                          m.group(2).strip()))
                            self.push(i, child)
                            child.parse(it)
                        case 'ifndef':
                            child = Scope(Macro(i, m.group(1),
                                          m.group(2).strip()))
                            self.push(i, child)
                            child.parse(it)
                        case 'if':
                            child = Scope(Macro(i, m.group(1),
                                          m.group(2).strip()))
                            self.push(i, child)
                            child.parse(it)
                        case 'elif':
                            self.macros.append(
                                Macro(i, m.group(1), m.group(2).strip()))
                            self.push(i)
                        case 'else':
                            self.macros.append(
                                Macro(i, m.group(1), ''))
                            self.push(i)
                        case 'endif':
                            self.push(i, Macro(i, 'endif', ''))
                            return
                        case 'define':
                            self.push(
                                i, Macro(i, m.group(1), m.group(2).strip()))
                        case 'include':
                            self.push(i, l)
                        case _:
                            self.push(i, l)
                else:
                    self.push(i, l)

            except StopIteration:
                break

    def do_resolve(self, definitions: List[str]):
        for macro in self.macros:
            for d in definitions:
                if d in macro.arg:
                    return True

    def write_to(self, w: io.IOBase, definitions: List[str] = [], *, level=0):
        # indent = '  ' * level
        any_prev = False

        resolve = self.do_resolve(definitions)
        for d in self.macros:
            if resolve:
                if not d.is_enable(definitions, any_prev):
                    # logging.debug(f'disable: {macro}: {definitions}')
                    continue
                any_prev = True
            else:
                if d.macro:
                    w.write(f'#{d.macro} {d.arg}\n')
            for child in d.lines:
                match child:
                    case Scope() as scope:
                        scope.write_to(w, definitions, level=level+1)

                    case Macro() as d:
                        # logger.debug(d)
                        if d.macro == 'endif' and resolve:
                            continue

                        w.write(f'#{d.macro} {d.arg}\n')
                        if d.macro == 'define':
                            splitted = d.arg.split(maxsplit=1)
                            definitions.append(splitted[0])

                    case str() as l:
                        w.write(l)
                        w.write('\n')

                    case _:
                        raise RuntimeError()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    src = pathlib.Path(sys.argv[1]).read_text()
    root = Scope(Macro(0, '', ''))
    root.parse(enumerate(src.splitlines()))

    dst = pathlib.Path(sys.argv[2])
    root.write_to(dst.open('w'), ['NANOVG_GL3_IMPLEMENTATION'])
