import sys

def replace_tabs(filename):
	try:
		with open(filename, 'r') as file:
			lines = file.readlines()
		
		with open(filename, 'w') as file:
			for line in lines:
				if line.startswith("//"):
					file.write("//")
					line = line[2:]

				while line.startswith('    '):
					file.write("\t")
					line = line[4:]
				file.write(line)
		
		print("Замена успешно выполнена.")
	except FileNotFoundError:
		print("Файл не найден.")


if __name__ == "__main__":
	if len(sys.argv) != 2:
		print(f"Использование: python {sys.argv[0]} <filename>")
		exit()

	filename = sys.argv[1]
	replace_tabs(filename)

