from typing import Optional, List, Iterator, Union, Tuple, NamedTuple
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
        self.lines: List[Union[str, 'Scope']] = []
        #
        self.end = None

    def __str__(self) -> str:
        return f'[#{self.macro} {self.arg}, {self.start}: {self.end}]'

    def push(self, end: int, l: Union[str, 'Scope']):
        self.end = end + 1
        self.lines.append(l)


class Scope:
    def __init__(self, macro: Macro) -> None:
        self.macros: List[Macro] = [macro]

    def __str__(self) -> str:
        return f'{", ".join(str(m) for m in self.macros)}'

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
                            self.macros[-1].push(i, child)
                            child.parse(it)
                        case 'ifndef':
                            child = Scope(Macro(i, m.group(1),
                                          m.group(2).strip()))
                            self.macros[-1].push(i, child)
                            child.parse(it)
                        case 'if':
                            child = Scope(Macro(i, m.group(1),
                                          m.group(2).strip()))
                            self.macros[-1].push(i, child)
                            child.parse(it)
                        case 'elif':
                            self.macros.append(
                                Macro(i, m.group(1), m.group(2).strip()))
                            self.macros[-1].push(i, l)
                        case 'else':
                            self.macros.append(
                                Macro(i, m.group(1), ''))
                            self.macros[-1].push(i, l)
                        case 'endif':
                            self.macros[-1].push(i, l)
                            return
                        case 'define':
                            self.macros[-1].push(i, l)
                        case 'include':
                            self.macros[-1].push(i, l)
                        case _:
                            self.macros[-1].push(i, l)
                else:
                    self.macros[-1].push(i, l)

            except StopIteration:
                break

    def print(self, definitions: List[str] = [], *, level=0):
        indent = '  ' * level
        print(f'{indent}{self}')
        for macro in self.macros:
            for child in macro.lines:
                match child:
                    case Scope() as scope:
                        scope.print(definitions, level=level+1)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    text = pathlib.Path(sys.argv[1]).read_text()
    root = Scope(Macro(0, '', ''))
    root.parse(enumerate(text.splitlines()))
    root.print(['NANOVG_GL2_IMPLEMENTATION'])
