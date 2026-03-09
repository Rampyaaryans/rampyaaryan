class Rampyaaryan < Formula
  desc "India's First Hinglish Programming Language"
  homepage "https://rampyaaryans.github.io/rampyaaryan/"
  url "https://github.com/Rampyaaryans/rampyaaryan/archive/refs/tags/v3.5.2.tar.gz"
  sha256 "REPLACE_WITH_ACTUAL_SHA256"
  license "LicenseRef-Rampyaaryan"

  depends_on "gcc" => :build

  def install
    system "make", "release"
    bin.install "bin/rampyaaryan"
  end

  test do
    (testpath/"hello.ram").write('likho "Namaste Duniya!"')
    assert_match "Namaste Duniya!", shell_output("#{bin}/rampyaaryan #{testpath}/hello.ram")
  end
end
