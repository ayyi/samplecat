using GLib;

public class Player : GLib.Object
{
	public signal void play();
	public signal void stop();

	construct
	{
	}

	public Player()
	{
		print("Player\n");
	}
}

