
board = "board.bmp", 128, 160
note = "note.bmp", 32, 20

def read_bmp(info):
  filename, width, height = info
  with open(filename, 'rb') as f:
    bts = f.read()[70:] # skip header
  print(f'{len(bts)=}')
  assert len(bts) == width * height * 2
  
  # now we read row-by-row from bottom to top because bmp weirdness
  real = ''
  for i in range(height - 1, -1, -1):
    row_start = i * width * 2
    row = bts[row_start:row_start + width * 2]
    highs = row[1::2]
    lows = row[::2]
    assert len(highs) + len(lows) == len(row)
    nums = []
    for high, low in zip(highs, lows):
      nums.append(hex((high << 8) + low))
    real += ", ".join(nums)
    real += ",\n"
  

  contents = real
  print(f'{len(contents)=}')
  with open(filename[:-3] + 'txt', 'w') as outf:
    outf.write(contents)


def main():
  for fname in [board, note]:
    read_bmp(fname)

if __name__ == "__main__":
  main()
