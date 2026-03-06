import subprocess
import os
import tempfile
from collections import Counter

def run_test(input_lines, expected_output):
    input_text = '\n'.join(input_lines)
    if input_lines:
        input_text += '\n'
    
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
        f.write(input_text)
        f.flush()
        
        result = subprocess.run(
            ['./sleepsort'],
            stdin=open(f.name),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
        
        os.unlink(f.name)
        
        output = result.stdout
        
        if not output:
            output_lines = []
        else:
            output_lines = output.split('\n')
            if output_lines and output_lines[-1] == '':
                output_lines.pop()
        
        if len(output_lines) != len(expected_output):
            print(u"FAIL: expected {} lines, got {}".format(len(expected_output), len(output_lines)))
            print(u"  input: {}".format(input_lines))
            print(u"  got: {}".format(output_lines))
            print(u"  expected: {}".format(expected_output))
            return False
        
        output_lengths = [len(line) for line in output_lines]
        for i in range(len(output_lengths) - 1):
            if output_lengths[i] > output_lengths[i + 1]:
                print(u"FAIL: not sorted by length")
                print(u"  input: {}".format(input_lines))
                print(u"  got: {}".format(output_lines))
                return False
        
        if output_lines != expected_output:
            print(u"FAIL: wrong output")
            print(u"  input: {}".format(input_lines))
            print(u"  got: {}".format(output_lines))
            print(u"  expected: {}".format(expected_output))
            return False
        
        print(u"PASS")
        return True

def test_ordering():
    test_cases = [
        (["a", "bb", "ccc"], ["a", "bb", "ccc"]),
        (["ccc", "bb", "a"], ["a", "bb", "ccc"]),
        (["hello", "hi", "greetings"], ["hi", "hello", "greetings"]),
        (["x", "xx", "x", "xx"], ["x", "x", "xx", "xx"]),
        ([], []),
        (["single"], ["single"]),
        (["", "a", ""], ["", "", "a"]),
        (["a", "", "aa"], ["", "a", "aa"]),
        (["", "", ""], ["", "", ""]),
        ([""], [""]),
        (["", ""], ["", ""]),
    ]
    
    for i, (input_lines, expected) in enumerate(test_cases):
        print(u"Test {}: ".format(i), end="")
        run_test(input_lines, expected)

def test_stress():
    lines = []
    for i in range(100):
        lines.append("x" * (i % 10 + 1) + str(i))
    
    print(u"Stress test (100 lines): ", end="")
    
    input_text = '\n'.join(lines)
    if lines:
        input_text += '\n'
    
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
        f.write(input_text)
        f.flush()
        
        result = subprocess.run(
            ['./sleepsort'],
            stdin=open(f.name),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
        
        os.unlink(f.name)
        
        output = result.stdout
        if not output:
            output_lines = []
        else:
            output_lines = output.split('\n')
            if output_lines and output_lines[-1] == '':
                output_lines.pop()
        
        if Counter(output_lines) != Counter(lines):
            print(u"FAIL: missing or extra strings")
            return False
        
        output_lengths = [len(line) for line in output_lines]
        for i in range(len(output_lengths) - 1):
            if output_lengths[i] > output_lengths[i + 1]:
                print(u"FAIL: not sorted by length")
                return False
        
        print(u"PASS")
        return True

if __name__ == "__main__":
    test_ordering()
    test_stress()