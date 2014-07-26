class Preprocessor
  def input
    $stdin
  end

  def output
    non_empty_lines.map do |line|
      if line =~ /^LDF @(\w+)$/
        "LDF #{line_number_of($1)}\n"
      elsif line =~ /^TSEL @(\w+) @(\w+)$/
        "TSEL #{line_number_of($1)} #{line_number_of($2)}\n"
      elsif line =~ /^SEL @(\w+) @(\w+)$/
        "SEL #{line_number_of($1)} #{line_number_of($2)}\n"
      else
        line
      end
    end.join
  end

  def lines
    @lines ||= input.lines.to_a
  end

  def non_empty_lines
    lines.select { |line| line.strip.length > 0 }
  end

  def line_number_of(label)
    labels[label] or raise "Unknown label: #{label}"
  end

  def labels
    @labels ||= Hash[labels_with_line_numbers]
  end

  def labels_with_line_numbers
    non_empty_lines.each_with_index
      .map { |line, number| [$1, number] if line =~ /\; @(\w+)/ }
      .compact
  end
end

puts Preprocessor.new.output