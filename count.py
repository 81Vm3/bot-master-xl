import os

def count_lines_in_file(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        return sum(1 for _ in f)

def count_cpp_and_h_lines(directory):
    total_lines = 0
    file_count = 0
    for root, _, files in os.walk(directory):
        for filename in files:
            if filename.endswith(('.cpp', '.h')):
                full_path = os.path.join(root, filename)
                lines = count_lines_in_file(full_path)
                total_lines += lines
                file_count += 1
                print(f"{full_path}: {lines} lines")
    print(f"\nScanned {file_count} files.")
    print(f"Total lines of code: {total_lines}")

if __name__ == "__main__":
    count_cpp_and_h_lines("src")
