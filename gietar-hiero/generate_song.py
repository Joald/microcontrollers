
def sweet_child_o_mine(out):
  # Sweet Child O' Mine by Guns N Roses is a really easy song to transcribe for this,
  # as the intro riff has little (and very structured) variations.
  # This code plays through the progression three times, but last two measures of the third
  # iteration are replaced by the outro defined below (thus, it's true to the album).

  columns = '4324232'.join(str(i) for i in [1, 1, 2, 2, 3, 3, 1, 1]) + '4324232'
  letters = '2977969'.join(str(i) for i in [2, 2, 4, 4, 7, 7, 2, 2]) + '2977767'
  octaves = '4334343'.join(str(i) for i in [3, 3, 3, 3, 3, 3, 3, 3]) + '4334343'
  duration = 30  
  intervals = [duration] * len(columns)

  start = 10

  output_i = lambda i: f'  {{.column = {columns[i]}, .start_time = {start}, .note = {{.letter = {letters[i]}, .octave = {octaves[i]}}}, .duration = {duration - 1}}},\n'

  for cutoff in [0, 0, 16]:
    for i in range(len(columns) - cutoff):
      out(output_i(i))
      start += intervals[i]

  song_len = len(columns) * 3 - 16
  columns = '42321212324232123'
  song_len += len(columns)
  
  letters = '79694929496979692'
  octaves = '43' * (len(columns) // 2) + '4'

  for i in range(len(columns)):
    if i == len(columns) - 1:
      # last note is longer (1/2 note instead of 1/8)
      duration *= 4
    out(output_i(i))
    start += intervals[i]
  


  out(f'// {song_len} notes recorded')


def main():
  with open('song.txt', 'w') as f:
    sweet_child_o_mine(f.write)


if __name__ == "__main__":
  main()